// -*- c-basic-offset: 4 -*-
/*
 * tdmapullscheduler.{cc,hh} - tdmapullscheduler dervied from
 * 	Ashish Sharma
 * 	Copyright (c) 2008-2009 University of California Santa Barbara
 *
 * 	This element was built as a derivative of 
 * 	priosched.{cc,hh} -- priority scheduler element
 * 	by Robert Morris, Eddie Kohler
 *
 * 	Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * 	Copyright (c) 2003 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <click/error.hh>
#include <click/confparse.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
#include <click/standard/storage.hh>
#include <elements/standard/notifierqueue.hh>
#include <elements/standard/quicknotequeue.hh>
#include <click/elemfilter.hh>
#include <click/router.hh>

#include "mdmacpullscheduler.hh"
CLICK_DECLS

MdmacPullScheduler::MdmacPullScheduler()
    : _signals(0)
{
    _max_tx_in_a_slot = 100;
    _tx_done_in_slot = 0;
}

MdmacPullScheduler::~MdmacPullScheduler()
{
}

int 
MdmacPullScheduler::initialize(ErrorHandler *errh)
{
    if (!(_signals = new NotifierSignal[ninputs()]))
	return errh->error("out of memory!");
    for (int i = 0; i < ninputs(); i++)
	_signals[i] = Notifier::upstream_empty_signal(this, i, 0);

    // find Queues upstream
    _queues.clear();
    _queue_elements.clear();
    if (!_queue_elements.size()) {
        CastElementFilter filter("Storage");
	int ok;
	for (int port = 0; port < ninputs(); port++) {
	    ok = router()->upstream_elements(this, port, &filter, _queue_elements);
	    if (ok < 0)
		return errh->error("flow-based router context failure");
	}
        filter.filter(_queue_elements);
    }
    
    if (_queue_elements.size() == 0)
        return errh->error("no Queues upstream");
    for (int i = 0; i < _queue_elements.size(); i++)
        if (Storage *s = (Storage *)_queue_elements[i]->cast("Storage"))
            _queues.push_back(s);
//   	    click_chatter("The thing that you wanted to see - _queue_elements[i]->name().c_str() - is %s", _queue_elements[i]->name().c_str());
        else
            errh->error("`%s' is not a storage element", _queue_elements[i]->name().c_str());
    if (_queues.size() != _queue_elements.size())
        return -1;
    click_chatter("mdmacpullscheduler: Size of upstream queues is %d", _queues.size());
    click_chatter("mdmacpullscheduler: Size of input queues is %d",  ninputs());
    //click_chatter("The thing _queue_elements[i]->name().c_str() is %s", _queue_elements[i]->name().c_str());
    return 0;
}

void
MdmacPullScheduler::cleanup(CleanupStage)
{
    delete[] _signals;
}

int
MdmacPullScheduler::configure(Vector<String> &conf, ErrorHandler *errh)
{
    _debug = false;
    if (cp_va_kparse(conf, this, errh,
		     // TDMA
		     // All values are in ms
		     // Timecycle should be a divisor of 1000
		     "FRAME_SIZE", cpkM, cpUnsigned, &_frame_size,   
		     "NUMBER_OF_SLOTS", cpkM, cpUnsigned, &_number_of_slots,
		     "SLOT_DURATION", cpkM, cpUnsigned, &_slot_duration,
		     "MAX_TX_IN_SLOT", cpkM, cpInteger, &_max_tx_in_a_slot,
		     "DEBUG", 0, cpBool, &_debug,
		     cpEnd) < 0)
	return -1;
    
    return 0;
}

//Initialize the table to store which slot is for which neighbor

int slotsToNeighbors[5] = {0, 1, 2, 0, 1};
int slotsPerNeighbor[3] = {2, 2, 1};
int _num_neighbors = 3;

//determine which is the current slot


//void MdmacPullScheduler::get_neighbor(uint32_t _frame_size, uint32_t _number_of_slots, uint32_t _slot_duration, Timestamp &now, uint32_t &cur_ms) 
//{
//	Timestamp now = Timestamp::now(); //Gets current time
//	uint32_t cur_ms = Timestamp::subsec_to_msec(now.subsec()); //Converts current time in ms
//   	int slot, _neigh_to_tx_to;
//    	slot = (((cur_ms%_frame_size)/_slot_duration)%_number_of_slots); //Calculates the current slot
//    	_neigh_to_tx_to = slotsToNeighbors[slot]; //returns the neighbor to be sent to in this slot
//}
int MdmacPullScheduler::get_neighbor(uint32_t _frame_size, uint32_t _number_of_slots, uint32_t _slot_duration) 
{
	Timestamp now = Timestamp::now(); //Gets current time
	uint32_t cur_ms = Timestamp::subsec_to_msec(now.subsec()); //Converts current time in ms
   	int slot;
    	slot = (((cur_ms%_frame_size)/_slot_duration)%_number_of_slots); //Calculates the current sloti
	click_chatter("Slot %d", slot+1);
    	return slotsToNeighbors[slot]; //returns the neighbor to be sent to in this slot
}


Packet *
MdmacPullScheduler::pull(int port)
{
//    Timestamp now = Timestamp::now(); //Gets current time
//    uint32_t cur_ms = Timestamp::subsec_to_msec(now.subsec()); //Converts current time in ms
//    click_chatter("########################################");
//    click_chatter("I am starting pull at now  %u:",  cur_ms);
    Packet *p;
//A kolko chesto da proverqvam v koi slot sym???
//Pyrvo proveri v koi slot si, posle proveri dali v tozi slot ima neshto i togava pullvai!!!
    bool input_available = false;
    int num_inputs = ninputs(); //I don't need this. In my implementation this is equal to the number of neighbors
//Tazi chast loop-va vsichki inputi i tyrsi takyv, v koito ima paketi. Pyrviqt nameren pylen input break-va for-a
    int i = 0;
    for (i = 0; i < num_inputs; i++) {
	if (_signals[i]) {
	    if (_queues[i]->size()) {
		input_available = true;
		break;
	    }
	    else {
		//click_chatter("Calling null pull in scheduler on input %d", i);
		if (p = (input(i).pull())) {
		    click_chatter("Ops!! Null pull fetched me a real packet on %d", i);
		}
	    }
	}
    }
//Ako nqma nito edna pylna opashka, vyrni 0. Ako tova ima pylna opashka, t.e. input_available == true shte prodylvi natam.
    if (false == input_available) {// || _next_resv_at_ts > now) {
	return 0;
    }
//Calculates the current slot
int _neigh_to_tx_to = get_neighbor(_frame_size, _number_of_slots, _slot_duration);
click_chatter("Neighbor %d", _neigh_to_tx_to);

    //m/Here should be the get_slot execution and assignment a neighbor for transmission 
    // TDMA
    
//    if ( (cur_ms % _timecycle >= _offset) && 
//	 (cur_ms % _timecycle < _offset + _duration) )
//	{
//Pulls packets	
	click_chatter("Number of inputs %d, requested neighbor %d", num_inputs, _neigh_to_tx_to);
	    if ( _tx_done_in_slot < _max_tx_in_a_slot) {
		if (_neigh_to_tx_to == 2){
//		for (int i = 0; i < ninputs(); i++)
//		    {
			if (_signals[_neigh_to_tx_to] && (p = input(_neigh_to_tx_to).pull()))
			    {
//TO DO				if (_debug) {
//TO DO				    click_chatter("%{timestamp}:RELEASE %{timestamp}  Cur_ms = %u, TimeCycle = %u,  NumberOfSlots = %u,  SlotDuration = %u, MAX_TX = %u, TX = %u", &now, &p->timestamp_anno(), cur_ms, _frame_size, _number_of_slots, _slot_duration, _max_tx_in_a_slot, _tx_done_in_slot); 
//TO DO				}
				_tx_done_in_slot++;
				return p;
			    }
//		    } //this is for the loop
	 	    } //this is for my verification
	    } 
//	}
//    else
//	{
	    // We are NOT within our designated slot
	    //click_chatter("%{timestamp}: Task invoked outside slot", &now);
	    _tx_done_in_slot = 0;
//	    return 0;
//	}
    return 0;
}


CLICK_ENDDECLS
EXPORT_ELEMENT(MdmacPullScheduler)
ELEMENT_MT_SAFE(MdmacPullScheduler)
