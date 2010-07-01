/*
 * =c
 * MdmacTime. Developed for the needs of MdmacScheduler.
 * Documentation by Mariya Zheleva
 *
 * MdmacTime maintains all logic related to 
 * time, slot and neighbor computations.
 *
 */

#ifndef MDMACTIME_HH_
#define MDMACTIME_HH_
#include <stdint.h>
#include <stdlib.h>
#include <click/element.hh>
#include <click/notifier.hh>

class MdmacTime : public Element { 
 public:
   
   MdmacTime(uint32_t frame_size, uint32_t number_of_slots, 
                    uint32_t slot_duration, int* neighbors);
   MdmacTime();
   ~MdmacTime();
  // Standard initialization of all Element derivatives in CLICK.
  const char *class_name() const	{ return "MdmacTime"; }
  const char *port_count() const	{ return "-/1"; }
  const char *processing() const	{ return PULL; }

  // Compute neighbor based on current time, slot duration, frame 
  // duration and number of slots per frame. 
  int get_neighbor(Timestamp &now, int &slot, float &inSlot_advancement);
  // Calculate the time difference between two points in the program.
  uint32_t diff(Timestamp start, Timestamp end);
  // TDMA frame size in ms.
  uint32_t _frame_size;
  // Number of slots in the frame.
  uint32_t _number_of_slots;
  // Duration of one slot in ms.
  uint32_t _slot_duration;
  // Pointer to array with slot-to-neighbor associations.
  int *_neighbors;
};

#endif
