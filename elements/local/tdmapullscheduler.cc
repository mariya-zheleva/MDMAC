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

#include "tdmapullscheduler.hh"
CLICK_DECLS

TdmaPullScheduler::TdmaPullScheduler()
    : _signals(0)
{
    _max_tx_in_a_slot = 100;
    _tx_done_in_slot = 0;
}

TdmaPullScheduler::~TdmaPullScheduler()
{
}

int 
TdmaPullScheduler::initialize(ErrorHandler *errh)
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
        else
            errh->error("`%s' is not a torage element", _queue_elements[i]->name().c_str());
    if (_queues.size() != _queue_elements.size())
        return -1;
    click_chatter("tdmapullsched: Size of upstream queues is %d", _queues.size());
    click_chatter("tdmapullscheduler: Size of input queues is %d",  ninputs());
    return 0;
}

void
TdmaPullScheduler::cleanup(CleanupStage)
{
    delete[] _signals;
}

int
TdmaPullScheduler::configure(Vector<String> &conf, ErrorHandler *errh)
{
    _debug = false;
    if (cp_va_kparse(conf, this, errh,
		     // TDMA
		     // All values are in ms
		     // Timecycle should be a divisor of 1000
		     "TIMECYCLE", cpkM, cpUnsigned, &_timecycle,   
		     "SLOT_START_OFFSET", cpkM, cpUnsigned, &_offset,
		     "SLOT_DURATION", cpkM, cpUnsigned, &_duration,
		     "MAX_TX_IN_SLOT", cpkM, cpInteger, &_max_tx_in_a_slot,
		     "DEBUG", 0, cpBool, &_debug,
		     cpEnd) < 0)
	return -1;
    
    return 0;
}

Packet *
TdmaPullScheduler::pull(int)
{
    Packet *p;

    bool input_available = false;
    int num_inputs = ninputs();
    
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
		    click_chatter("Shit!! Null pull fetched me a real packet on %d", i);
		}
	    }
	}
    }

    if (false == input_available) {// || _next_resv_at_ts > now) {
	return 0;
    }
    
    // TDMA
    Timestamp now = Timestamp::now();
    uint32_t cur_ms = Timestamp::subsec_to_msec(now.subsec());
    
    if ( (cur_ms % _timecycle >= _offset) && 
	 (cur_ms % _timecycle < _offset + _duration) )
	{
	    if ( _tx_done_in_slot < _max_tx_in_a_slot) {
		for (int i = 0; i < ninputs(); i++)
		    {
			if (_signals[i] && (p = input(i).pull()))
			    {
				if (_debug) {
				    click_chatter("I am releasing a packet");
				    click_chatter("%{timestamp}: RELEASE %{timestamp}  Cur_ms = %u, TimeCycle = %u,  Offset = %u,  Duration = %u, MAX_TX = %u, TX = %u", &now, &p->timestamp_anno(), cur_ms, _timecycle, _offset, _duration, _max_tx_in_a_slot, _tx_done_in_slot); 
				}
				_tx_done_in_slot++;
				return p;
			    }
		    }
	    }
	}
    else
	{
	    // We are NOT within our designated slot
	    //click_chatter("%{timestamp}: Task invoked outside slot", &now);
	    _tx_done_in_slot = 0;
	    return 0;
	}
    return 0;
}


CLICK_ENDDECLS
EXPORT_ELEMENT(TdmaPullScheduler)
ELEMENT_MT_SAFE(TdmaPullScheduler)
