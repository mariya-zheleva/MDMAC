// -*- c-basic-offset: 4 -*-
#ifndef CLICK_MDMACPULLSCHEDULER_HH
#define CLICK_MDMACPULLSCHEDULER_HH
#include <click/element.hh>
#include <click/notifier.hh>
class Storage;

CLICK_DECLS

/*
 * =c
 * MdmacPullScheduler derived from 
 * 	PrioSched
 * 	Documentation pending - Mariya
 *
 * =s scheduling
 * pulls from priority-scheduled inputs
 * =d
 * Each time a pull comes in the output, PrioSched pulls from
 * each of the inputs starting from input 0.
 * The packet from the first successful pull is returned.
 * This amounts to a strict priority scheduler.
 *
 * The inputs usually come from Queues or other pull schedulers.
 * PrioSched uses notification to avoid pulling from empty inputs.
 *
 * =a Queue, RoundRobinSched, StrideSched, DRRSched, SimplePrioSched
 */

class MdmacPullScheduler : public Element { public:
  
    MdmacPullScheduler();
    ~MdmacPullScheduler();
  
    const char *class_name() const	{ return "MdmacPullScheduler"; }
    const char *port_count() const	{ return "-/1"; }
    const char *processing() const	{ return PULL; }

    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void cleanup(CleanupStage);
    //void add_handlers();

    Packet *pull(int port);

    // TDMA
    uint32_t _frame_size;
    uint32_t _number_of_slots;
    uint32_t _slot_duration;
    uint32_t _tx_done_in_slot;
    uint32_t _max_tx_in_a_slot;

//    int get_slot(uint32_t _frame_size, uint32_t _number_of_slots, uint32_t _slot_duration);
    int get_neighbor(uint32_t _frame_size, uint32_t _number_of_slots, uint32_t _slot_duration);
//    int get_neighbor(int slot);
//TO DO/    int init_table();    

    bool _debug;
    //static String read_param(Element *, void *);
    //static int write_handler(const String &, Element *, void *, ErrorHandler *);
  private:
    
    NotifierSignal *_signals;
    Vector<Storage *> _queues;
    Vector<Element *> _queue_elements;  
};

CLICK_ENDDECLS
#endif
