// -*- c-basic-offset: 4 -*-
/*
 *  mdmacscheduler.{cc,hh} - dervied from Mariya Zheleva
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
#include "mdmacscheduler.hh"
#include "mdmactime.hh"

CLICK_DECLS

MdmacScheduler::MdmacScheduler()
    : _signals(0)
{
    _tx_done_in_slot = 0;
    _last_slot = -99;
}

MdmacScheduler::~MdmacScheduler()
{
}

int 
MdmacScheduler::initialize(ErrorHandler *errh)
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
            errh->error("`%s' is not a storage element", _queue_elements[i]->name().c_str());
    if (_queues.size() != _queue_elements.size())
        return -1;
    return 0;
}

void
MdmacScheduler::cleanup(CleanupStage)
{
    delete[] _signals;
    delete _logger;
    delete _time_manager;
}

int
MdmacScheduler::configure(Vector<String> &conf, ErrorHandler *errh)
{
    _log = true;                  
    
    uint32_t frame_size;
    uint32_t number_of_slots;
    uint32_t slot_duration;
 
    if (cp_va_kparse(conf, this, errh,
		     "FRAME_SIZE", cpkM, cpUnsigned, &frame_size,   //in micro-seconds
		     "NUMBER_OF_SLOTS", cpkM, cpUnsigned, &number_of_slots,
		     "SLOT_DURATION", cpkM, cpUnsigned, &slot_duration,  //in micro-seconds
		     "BIT_RATE", cpkM, cpInteger, &_bit_rate,        //in Mbps
		     "PACKET_SIZE", cpkM, cpInteger, &_packet_size,  //in bytes
		     "DEBUG", 0, cpBool, &_log,
		     cpEnd) < 0)
	    return -1;
    //For now this is hard-coded. TODO: add to configuration.
    int SlotsToNeighbors[5] = {1, 0, 1, 1, 1}; 
    // Create a MDMAC time manager object. It maintains all logic related to 
    // time, slot and neighbor computations.
    _time_manager = new MdmacTime(frame_size, number_of_slots, 
                    slot_duration, SlotsToNeighbors);
    uint32_t pps, aps;
    PacketsPerSlot(aps, pps);
    _max_tx_per_slot = pps; 
    float ack_thr = 0, pack_thr = 0;
    TXTimeThreshold(ack_thr, pack_thr); 
    // Create a MDMAC logger object. It handles all the reporting for MDMAC 
    // performance evaluation.
    _logger->log_message("In-slot thresholds: ack_threshold ", (float) ack_thr);
    _logger->log_message("In-slot thresholds: pack_threshold ", (float) pack_thr);
    if(_log){
      _logger = new MdmacLogger(frame_size, number_of_slots, 
          slot_duration, _bit_rate, _packet_size, _max_tx_per_slot, 
          pack_thr);
    }
    return 0;
}

// Calculate the maximum number of packet transmissions per slot.
uint32_t MdmacScheduler::PacketsPerSlot(uint32_t &_aps, uint32_t &_pps)
{
  _pps = (float) ((float)_time_manager->_slot_duration/1000)/
        ((float) (_packet_size*8)/(_bit_rate*1000000));
  _aps = _pps + 1;
  return 0;
}

// Calculate the in-slot time threshold for packet transmission. 
float MdmacScheduler::TXTimeThreshold(float &_ack_threshold, float &_pack_threshold)
{
  uint32_t pps, aps;
  PacketsPerSlot(aps, pps);
  // 800 is the size of an ACK in bits. 
  // 0.00025ms is the time for ACK generation at the resceiver. 
  // The second term is time to send one packet in ms.
  float ack_time = aps*(((float) 800/(_bit_rate*1000000)) + 0.00025);     // That's the time to generate and send one ack
  float pack_time = aps*(((float) 800/(_bit_rate*1000000)) + 0.00025) + // That's the time to generate and send one ack
    1000*((float) (_packet_size*8)/(_bit_rate*1000000));                 // That's the time to send one packet in ms
  _pack_threshold = _time_manager->_slot_duration - pack_time - 0.5;
  _ack_threshold = _time_manager->_slot_duration - ack_time - 0.5;
  return 0;
}

bool transmit_acks = false;    

Packet *MdmacScheduler::pull(int )
{
    Packet *p;
    bool input_available = false;
    int num_inputs = ninputs(); 

    // Computes in'slot time thresholds for ACK and packet transmission.
    float ack_thr = 0, pack_thr = 0;
    TXTimeThreshold(ack_thr, pack_thr); 

    // Comutes the max number of ACK and packet transmissions per slot.
    uint32_t max_pps, max_aps;
    PacketsPerSlot(max_aps, max_pps);

   // If all upstream queues are empty, pull() will break here.
    int i = 0;
    for (i = 0; i < num_inputs; i++) {
        if (_signals[i]) {
		      if (_queues[i]->size()) {
	       	        input_available = true;
                	break;
          }
        	else {
			      if (p = (input(i).pull())) {
              _logger->log_message("Error: Null pull "
                  "fetched me a real packet on ",(uint32_t) i);
			      }	
          }	
      }
    }
    // Exits if there are no packets awaiting in the upstream queues.
    if (false == input_available) {
        return 0;
    }

    // There is input in the queue, so compute slot and neighbor and 
    // perform pull.
    Timestamp now;
    int slot;
    float inSlot_advancement;
    int neighbor = _time_manager->get_neighbor(now, slot, inSlot_advancement);
    // This part is to handle pulling acks from the ACKQueues.
    
    
    if (true == transmit_acks){ // Transmit acknowledgements
      if ((slot == _last_slot) || (-99 == _last_slot)) {
        if (inSlot_advancement < ack_thr){
		      if ( _tx_done_in_slot < max_aps){
			      if (_signals[2*neighbor + 1] && 
                (p = input(2*neighbor + 1).pull())) {
              if (_log) {                           
                _logger->log_packet_pull(now, p->timestamp_anno(), slot, 
                          _tx_done_in_slot, neighbor, inSlot_advancement);
              }  
              _tx_done_in_slot++;
              _last_slot = slot;
              return p;
            }else{
            transmit_acks = false;
            }
			    }
        }
      } else { // Slot changed
          if (inSlot_advancement < ack_thr){
	          _tx_done_in_slot = 0;
	          _last_slot = slot;
		        if (_signals[2*neighbor] && 
              (p = input(2*neighbor).pull())) {
			        if (_log) {
                _logger->log_packet_pull(now, p->timestamp_anno(), slot, 
                        _tx_done_in_slot, neighbor, inSlot_advancement);
			        }
			      _tx_done_in_slot++;
			      return p;
		        }else{
            transmit_acks = false;
            }
          }
        }
    }else{ // Transmit packets
    // Determin if (i) slot changed, (ii) if the maximum number of packet 
    // transmissions per slot had been exceeded and (iii) if there is 
    // a packet pending for the calculated neighbor.
    
      if ((slot == _last_slot) || (-99 == _last_slot)) {
        if (inSlot_advancement < pack_thr){
		      if ( _tx_done_in_slot < max_pps){
			      if (_signals[2*neighbor] && 
                (p = input(2*neighbor).pull())) {
              if (_log) {                           
                _logger->log_packet_pull(now, p->timestamp_anno(), slot, 
                          _tx_done_in_slot, neighbor, inSlot_advancement);
              }  
              _tx_done_in_slot++;
              _last_slot = slot;
              //transmit_acks = true;
              return p;
            }  
			    }
        }
      } else { // Slot changed
          if (inSlot_advancement < pack_thr){
	          _tx_done_in_slot = 0;
	          _last_slot = slot;
		        if (_signals[2*neighbor] && 
              (p = input(2*neighbor).pull())) {
			        if (_log) {
                _logger->log_packet_pull(now, p->timestamp_anno(), slot, 
                        _tx_done_in_slot, neighbor, inSlot_advancement);
			        }
			      _tx_done_in_slot++;
            //transmit_acks = true;
			      return p;
		        } 
	        return NULL;
	        }
      return NULL;
      }
    }  
    return NULL;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MdmacScheduler)
ELEMENT_MT_SAFE(MdmacScheduler)
