#ifndef MDMACDEBUGGER_HH_
#define MDMACDEBUGGER_HH_
#include <stdint.h>
#include <click/element.hh>
#include <click/notifier.hh>
#include "mdmacscheduler.hh"

class MdmacDebugger : public Element{ 
  
  public:
  MdmacDebugger();
  ~MdmacDebugger();
  const char *class_name() const	{ return "MdmacDebugger"; }
  const char *port_count() const	{ return "-/1"; }
  const char *processing() const	{ return PULL; }

  void debug(bool _debug, Timestamp now, Timestamp pack_release, uint32_t current_ms, 
  uint32_t _frame_size, uint32_t _number_of_slots, uint32_t _slot_duration, 
  int slot, int _max_tx_per_slot, int _tx_done_in_slot, int _neigh_to_tx_to);
};

#endif
