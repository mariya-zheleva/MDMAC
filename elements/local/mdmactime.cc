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
#include "mdmactime.hh"

CLICK_DECLS

MdmacTime::MdmacTime()
{
}

MdmacTime::MdmacTime(uint32_t frame_size, uint32_t number_of_slots, 
                    uint32_t slot_duration, int *neighbors)
{
  _neighbors = new int[number_of_slots];
  for (uint32_t i=0; i<number_of_slots; i++){
  _neighbors[i] = neighbors[i];
  }
  _frame_size = frame_size;
  _number_of_slots = number_of_slots;
  _slot_duration = slot_duration;
}

MdmacTime::~MdmacTime()
{
}


int MdmacTime::get_neighbor(Timestamp &now, int &slot, float &inSlot_advancement)
{
    now = Timestamp::now();
    
    // Gets the subseconds value from the timestamp
    uint32_t cur_us = Timestamp::subsec_to_usec(now.subsec());
    /*TODO: this is for oct to dec conversion!!!!!!!
    long int current_ms;
    char *cur_ms = (char *) Timestamp::subsec_to_msec(now.subsec());
    current_ms = atol (cur_ms);
    */
    // Calculate which is the current slot.
    // Time is divided into frames (TDMA MAC). 
    // A frame is further divided into slots, allocated  
    // to communicating parties.
    //inSlot_advancement = (float) (cur_us%(_frame_size*1000))/_slot_duration;
    inSlot_advancement = ((cur_us%(_frame_size*1000))%(_slot_duration*1000))/1000;
//    click_chatter("InSlotAdvancement %f", inSlot_advancement);
    //slot = (((cur_us%(_frame_size*1000))/_slot_duration)%_number_of_slots);
    slot = (cur_us%(_frame_size*1000))/(_slot_duration*1000);
  //  click_chatter("Slot is: %d", slot);
    
    // Get the neighbor corresponding to the current slot. 
    return _neighbors[slot];
}


// Calculate the time difference between two points in the program.
uint32_t MdmacTime::diff(Timestamp start, Timestamp end)
{
  uint32_t start_us = Timestamp::subsec_to_usec(start.subsec());
  uint32_t end_us = Timestamp::subsec_to_usec(end.subsec());
  if(end_us < start_us)
    return (999999 + end_us) - start_us;
  else
    return end_us - start_us;
  return 999999999;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MdmacTime)
ELEMENT_MT_SAFE(MdmacTime)
