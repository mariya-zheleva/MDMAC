// -*- c-basic-offset: 4 -*-
#ifndef CLICK_MDMACSCHEDULER_HH
#define CLICK_MDMACSCHEDULER_HH
#include <click/element.hh>
#include <click/notifier.hh>
#include "mdmaclogger.hh"
#include "mdmactime.hh"
class Storage;
class MdmacLogger;
class MdmacTime;

CLICK_DECLS

/*
 * =c
 * MdmacScheduler derived from PrioSched
 * Documentation by Mariya Zheleva
 *
 * This is a scheduler for a TDMA MAC protocol for 60 GHz networks 
 * called MDMAC. 
 *
 * =s
 * Pulls from inputs based on computation of the current slot.
 * Restricts the number of packet transmissions per single slot, to 
 * assure slotted packet reception. 
 * The inputs usually come from Queues. MdmacScheduler uses notification to avoid 
 * pulling from empty inputs.
 * 
 * Keyword arguments are:
 *
 * =over 8
 *
 * =item FRAME_SIZE
 *
 * TDMA frame in ms.
 *
 * =item NUMBER_OF_SLOTS
 *
 * Number of slots that the frame consists of.
 *
 * =item SLOT_DURATION
 *
 * Duration of one slot in ms.
 *
 * =item BIT_RATE
 *
 * Packet transmission bit rate in Mbps.
 *
 * =item PACKET_SIZE
 *
 * Packet size in bytes.
 *
 * =item DEBUG
 *
 * Print a debug message.
 *
 * =back
 *
 * =e
 *
 * MdmacScheduler(FRAME_SIZE 20, NUMBER_OF_SLOTS 5, SLOT_DURATION 4, BIT_RATE 12, PACKET_SIZE 1560, DEBUG true)
 *
 * =d
 *
 * =a Queue, RoundRobinSched, StrideSched, DRRSched, SimplePrioSched
 */

class MdmacScheduler : public Element { public:
  
    MdmacScheduler();
    ~MdmacScheduler();
    // Standard initialization of all Element derivatives in CLICK.  
    const char *class_name() const	{ return "MdmacScheduler"; }
    const char *port_count() const	{ return "-/1"; }
    const char *processing() const	{ return PULL; }

    // Configure element with user defined arguments.
    int configure(Vector<String> &, ErrorHandler *);
    // Initialize element.
    int initialize(ErrorHandler *);
    // Compute the max number of packets and ACKs transmissions per slot.
    uint32_t PacketsPerSlot(uint32_t &_aps, uint32_t &_pps);
    // Compute time threshold for packets and ACKs transmission within the slot.
    float TXTimeThreshold(float &_ack_threshold, float &_pack_threshold);
    // Cleans memory.
    void cleanup(CleanupStage);

    Packet *pull(int port);
    MdmacLogger* _logger;
    MdmacTime* _time_manager;
    
    // The last slot in which there was packet transmission.
    int _last_slot;               
    // Counts packet transmissions within a slot.
    uint32_t _tx_done_in_slot;    
    // User defined.
    uint32_t _packet_size;        
    // User defined.
    uint32_t _bit_rate;           
    uint32_t _pps;               
    // User defined. Switches logging off and on.
    bool _log;                    
    // Packets Per Slot - indicates the max number of 
    // packets that can be transmitted in a slot.
    uint32_t _max_tx_per_slot;    
    // Sets thime threshold for packet transmission 
    // within the slot.
    float _inSlot_TX_threshold;   

  private:
    
    NotifierSignal *_signals;
    Vector<Storage *> _queues;
    Vector<Element *> _queue_elements;  
};

CLICK_ENDDECLS
#endif
