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
#include "mdmaclogger.hh"

CLICK_DECLS

MdmacLogger::MdmacLogger(uint32_t frame_size,
  uint32_t number_of_slots, 
  uint32_t slot_duration,
  uint32_t bit_rate,
  uint32_t packet_size,
  int max_tx_per_slot,
  int inSlot_TX_threshold)
    
{                                              
  _frame_size = frame_size;
  _number_of_slots = number_of_slots; 
  _slot_duration = slot_duration;
  _bit_rate = bit_rate;
  _packet_size = packet_size;
  _max_tx_per_slot = max_tx_per_slot;
  _inSlot_TX_threshold = inSlot_TX_threshold;   
}

MdmacLogger::~MdmacLogger()
{
}

// Logs message
void MdmacLogger::log_message(char *msg, uint32_t value)
{
  click_chatter("%s, %u", msg, value);
}

// Logs message 
void MdmacLogger::log_message(char *msg, float value)
{
  click_chatter("%s, %f", msg, value);
}

// Logs message when packet is pulled.
void MdmacLogger::log_packet_pull(Timestamp now, 
                    Timestamp pack_release, int slot, 
                    uint32_t _tx_done_in_slot, int _neigh_to_tx_to, 
                    float inSlot_advancement){
    click_chatter("%{timestamp}. RELEASE %{timestamp}. "
                  "TimeCycle = %u,  NumberOfSlots = %u,  "
                  "SlotDuration = %u, SlotNumber = %d, " 
                  "In-slot_advancement = %f, MAX_TX = %u, "
                  "TX = %u, Neighbor = %d",
                  &now, &pack_release,
                  _frame_size, _number_of_slots,
                  _slot_duration, slot, 
                  inSlot_advancement, _max_tx_per_slot,
                  _tx_done_in_slot, _neigh_to_tx_to);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MdmacLogger)
ELEMENT_MT_SAFE(MdmacLogger)
