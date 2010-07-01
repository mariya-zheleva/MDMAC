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
#include "mdmacdebugger.hh"

CLICK_DECLS

MdmacDebugger::MdmacDebugger()
{
}

MdmacDebugger::~MdmacDebugger()
{
}

void
MdmacDebugger::debug(bool _debug, Timestamp now, Timestamp pack_release, uint32_t current_ms, 
    uint32_t _frame_size, uint32_t _number_of_slots, uint32_t _slot_duration, 
    int slot, int _max_tx_per_slot, int _tx_done_in_slot, int _neigh_to_tx_to){
  if (_debug) {
    click_chatter("%{timestamp}. RELEASE %{timestamp}"
                  "Cur_ms = %u, Cur_us = %u,"
                  "TimeCycle = %u,  NumberOfSlots = %u,  "
                  "SlotDuration = %u, SlotNumber = %d, MAX_TX = %u, "
                  "TX = %u, Neighbor = %d",
                  now, pack_release, current_ms,
                  _frame_size, _number_of_slots,
                  _slot_duration, slot , _max_tx_per_slot,
                  _tx_done_in_slot, _neigh_to_tx_to);
  }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MdmacDebugger)
ELEMENT_MT_SAFE(MdmacDebugger)
