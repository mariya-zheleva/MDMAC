/*
 * =c
 * MdmacLogger. Developed for the needs of MdmacScheduler.
 * Documentation by Mariya Zheleva
 *
 * MdmacLogger handles logging of messages for pwerformance 
 * evaluation and debug purposes.
 *
 */

#ifndef MDMACLOGGER_HH_
#define MDMACLOGGER_HH_
#include <stdint.h>
#include <click/element.hh>
#include <click/notifier.hh>

class MdmacLogger : public Element{ 
  
  public:

  MdmacLogger(){};
  
  MdmacLogger(uint32_t frame_size,
  uint32_t number_of_slots, 
  uint32_t slot_duration,
  uint32_t bit_rate,
  uint32_t packet_size,
  int max_tx_per_slot,
  int inSlot_TX_threshold);

  ~MdmacLogger();

  // Standard initialization of all Element derivatives in CLICK.
  const char *class_name() const	{ return "MdmacLogger"; }
  const char *port_count() const	{ return "-/1"; }
  const char *processing() const	{ return PULL; }

  // TDMA frame size in ms.
  uint32_t _frame_size;
  // Number of slots in the frame.
  uint32_t _number_of_slots; 
  // Duration of one slot in ms.
  uint32_t _slot_duration;      
  // User defined.
  uint32_t _bit_rate;
  // User defined.
  uint32_t _packet_size;
  // max number of packets that can be transmitted in a slot.
  int _max_tx_per_slot;
  // Sets time threshold for packet transmission 
  // within the slot.
  int _inSlot_TX_threshold;
  
  // Logs message.
  void log_message(char *msg, uint32_t value);
  // Logs message.
  void log_message(char *msg, float value);
  // Logs message when packet is pulled.
  void log_packet_pull(Timestamp now, 
                    Timestamp pack_release, int slot, 
                    uint32_t _tx_done_in_slot, int _neigh_to_tx_to, 
                    float inSlot_advancement);
};

#endif
