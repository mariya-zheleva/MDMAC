/*
 * ackretrysender.{cc,hh} -- element buffers packets until acknowledged
 * Douglas S. J. De Couto
 *
 * Copyright (c) 2002 Massachusetts Institute of Technology
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
#include <click/glue.hh>
#include <clicknet/wifi.h>
#include <click/confparse.hh>
#include <click/packet.hh>
#include <click/error.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/straccum.hh>
#include "mdmacackretrysender.hh"
#include "mdmacackresponder.hh"
CLICK_DECLS

MdmacACKRetrySender::MdmacACKRetrySender()
  : _timeout(0), _max_tries(0),
    _num_tries(0), _history_length(500),
    _waiting_packet(0), _verbose(true),
    _timer(this), _task(this)
{
  _cansend = true; // TODO  MDMAC
}

MdmacACKRetrySender::~MdmacACKRetrySender()
{
}

void MdmacACKRetrySender::printarray (char *msg, uint8_t arg[], int length) {
  for (int n=0; n<length; n++)
  click_chatter("%s: %x", msg, arg[n]);
}

void
MdmacACKRetrySender::push(int port, Packet *p)
{
  assert(port == 1);
  check();

  // Need this for MDMAC. Are we waiting for an ack?
  if (!_waiting_packet) {
    // we aren't waiting for ACK
    if (_verbose)
      click_chatter("MdmacACKRetrySender %s: got unexpected ACK", name().c_str());
    p->kill();
    return;
  }
  
  click_wifi *w_ack = (click_wifi *) p->data();
  /*  
  // Need this for MDMAC. This verifies if the packet received is an ack.
  // TODO can add this back when you figure out the ACK type!!!
  if (ntohs(e_ack->ether_type) != ETHERTYPE_GRID_ACK) {
    click_chatter("MdmacACKRetrySender %d: got non-ACK packet on second input", name().c_str());
    p->kill();
    return;
  }
  */

  // was this response for the packet we have?
  click_wifi *w_waiting = (click_wifi *) _waiting_packet->data();
  /*
  click_chatter("ACK received: %x", w_ack);
  click_chatter("PACK SA: %x", w_waiting->i_addr2);
  click_chatter("PACK DA: %x", w_waiting->i_addr1);
  click_chatter("PACK seq#: %x", w_waiting->i_seq);
  */
  /*
  printarray("PACK SA", w_waiting->i_addr2, 6);
  printarray("PACK DA", w_waiting->i_addr1, 6);
  printarray("PACK seq#", w_waiting->i_seq, 2);

  printarray("ACK SA", w_ack->i_addr2, 6);
  printarray("ACK DA", w_ack->i_addr1, 6);
  printarray("ACK seq#", w_ack->i_seq, 2);
  */
  if (memcmp(w_ack->i_addr2, w_waiting->i_addr1, 6) ||// is SA_ACK same as the DA_PACK_cloned
      memcmp(w_ack->i_addr1, w_waiting->i_addr2, 6)){// ||  // is DA_ACK same as SA_PACK_cloned
// TODO      memcmp(w_ack->i_seq, w_waiting->i_seq, 2)){        // is the seq# that the ACK carries in its payload the same as the seq# of the packet cached
     if (_verbose) // no it wasn't
      click_chatter("MdmacACKRetrySender %s: got ACK for wrong packet", name().c_str());
    p->kill();
    return;
  } 

  // ahhh, ACK was for us.
  _history.push_back(tx_result_t(_waiting_packet->timestamp_anno(),
				 _num_tries, true));
  while (_history.size() > (int) _history_length)
      _history.pop_front();
  _waiting_packet->kill();
  _waiting_packet = 0;
  _num_tries = 0;
  _timer.unschedule();  // StopTimer  
  p->kill();
  _cansend = true;   // TODO  MDMAC

  check();
}


bool
MdmacACKRetrySender::run_task(Task *)
{
  check();

  _task.fast_reschedule(); // ?????

  if (_waiting_packet)
    return true;

  if(_cansend){ // TODO  // Added for MDMAC
     Packet *p = input(0).pull();     // GetPacket
     if (!p)
       return true;

     if (_max_tries > 1) {
       _waiting_packet = p->clone();              // CachePacket
       click_chatter("PACK cloned: %x", _waiting_packet); // MDMAC debug
       _num_tries = 1;
       _timer.schedule_after_msec(_timeout);     // SetTimer
     }

     check();
     output(0).push(p);     // SendPacket
     _cansend = false; 
  }
  return true;
}

int
MdmacACKRetrySender::configure(Vector<String> &conf, ErrorHandler *errh)
{
  _max_tries = 16;
  _timeout = 10;
  _verbose = true;
  _history_length = 500;
  int res = cp_va_kparse(conf, this, errh,
			 "MAX_TRIES", 0, cpUnsigned, &_max_tries,
			 "TIMEOUT", 0, cpUnsigned, &_timeout,
			 "VERBOSE", 0, cpBool, &_verbose,
			 "HISTORY_LEN", 0, cpUnsigned, &_history_length,
			 cpEnd);

  if (res < 0)
    return res;

  if (_timeout == 0)
    return errh->error("TIMEOUT must be > 0");
  if (_max_tries == 0)
    return errh->error("MAX_TRIES must be > 0");

  return 0;
}

int
MdmacACKRetrySender::initialize(ErrorHandler *errh)
{
  _timer.initialize(this);
  ScheduleInfo::join_scheduler(this, &_task, errh);

  check();
  return 0;
}

void
MdmacACKRetrySender::run_timer(Timer *)
{
  assert(_waiting_packet && !_timer.scheduled());

  Packet *p = _waiting_packet;

  if (_num_tries >= _max_tries) {
    _history.push_back(tx_result_t(p->timestamp_anno(), _num_tries, false));
    while (_history.size() > (int) _history_length)
      _history.pop_front();
    _waiting_packet->kill();
    _waiting_packet = p = 0;
    _num_tries = 0;
  }
  else {
    _timer.schedule_after_msec(_timeout);
    _waiting_packet = p->clone();
    _num_tries++;
  }

  check();

  if (p)
    output(0).push(p);
}

void
MdmacACKRetrySender::check()
{
  // check() should be called *before* most pushes() from element
  // functions, as each push may call back into the element.

  // if there is a packet waiting, the timeout timer should be running
  assert(_waiting_packet ? _timer.scheduled() : !_timer.scheduled());

  // no packet has been sent more than the max number of times
  assert(_num_tries <= _max_tries);

  // any waiting packet has been sent at least once
  assert(_waiting_packet ? _num_tries >= 1 : _num_tries == 0);
}

void
MdmacACKRetrySender::add_handlers()
{
  add_read_handler("history", print_history, 0);
  add_read_handler("summary", print_summary, 0);
  add_write_handler("clear", clear_history, 0);
}

String
MdmacACKRetrySender::print_history(Element *e, void *)
{
  MdmacACKRetrySender *a = (MdmacACKRetrySender *) e;
  StringAccum s;
  for (MdmacACKRetrySender::HistQ::const_iterator i = a->_history.begin();
       i != a->_history.end(); i++)
    s << i->pkt_time << "\t" << i->num_tx << "\t"
      << (i->success ? "succ" : "fail") << "\n";
  return s.take_string();
}

String
MdmacACKRetrySender::print_summary(Element *e, void *)
{
  MdmacACKRetrySender *a = (MdmacACKRetrySender *) e;
  unsigned num_succ = 0;
  unsigned num_fail = 0;
  unsigned sum_tx = 0;
  unsigned max_tx = 0;
  unsigned min_tx = 0;
  int n = a->_history.size();

  for (MdmacACKRetrySender::HistQ::const_iterator i = a->_history.begin();
       i != a->_history.end(); i++) {
    if (sum_tx == 0)
      max_tx = min_tx = i->num_tx;
    else {
      max_tx = max_tx > i->num_tx ? max_tx : i->num_tx;
      min_tx = min_tx < i->num_tx ? min_tx : i->num_tx;
    }
    if (i->success)
      num_succ++;
    else
      num_fail++;
    sum_tx += i->num_tx;
  }

  unsigned txc = 0;
  if (n > 0)
    txc = (1000 * sum_tx) / n;

  StringAccum s;
  s << "packets: " << n << "\n"
    << "success: " << num_succ << "\n"
    << "fail: " << num_fail << "\n"
    << "min_txc: " << min_tx << "\n"
    << "max_txc: " << max_tx << "\n"
    << "avg_txc: " << cp_unparse_real10(txc, 3) << "\n";
  return s.take_string();
}

int
MdmacACKRetrySender::clear_history(const String &, Element *e, void *, ErrorHandler *)
{
  MdmacACKRetrySender *a = (MdmacACKRetrySender *) e;
  a->_history.clear();
  return 0;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MdmacACKRetrySender)
