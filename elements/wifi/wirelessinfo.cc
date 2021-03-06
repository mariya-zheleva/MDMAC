/*
 * wirelessinfo.{cc,hh} -- Poor man's arp table
 * John Bicket
 *
 * Copyright (c) 2003 Massachusetts Institute of Technology
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
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
#include <clicknet/ether.h>
#include "wirelessinfo.hh"
#include <click/router.hh>
CLICK_DECLS

WirelessInfo::WirelessInfo()
  : _ssid(""),
    _bssid((const unsigned char *) "\000\000\000\000\000\000"),
    _channel(-1),
    _interval(100),
    _wep(false)
{

}

WirelessInfo::~WirelessInfo()
{
}

int
WirelessInfo::configure(Vector<String> &conf, ErrorHandler *errh)
{
  int res;
  reset();
  res = cp_va_kparse(conf, this, errh,
		     "SSID", 0, cpString, &_ssid,
		     "BSSID", 0, cpEthernetAddress, &_bssid,
		     "CHANNEL", 0, cpInteger, &_channel,
		     "INTERVAL", 0, cpInteger, &_interval,
		     "WEP", 0, cpBool, &_wep,
#if CLICK_NS
		     "IFID", 0, cpInteger, &_ifid,
#endif
		     cpEnd);

#if CLICK_NS
  // nletor - change interface number ifid
  // to correct channel in simulator
  if (_ifid >= 0)
      simclick_sim_command(router()->simnode(), SIMCLICK_CHANGE_CHANNEL, _ifid, _channel);
#endif

  return res;
}


enum {H_SSID, H_BSSID, H_CHANNEL, H_INTERVAL, H_WEP, H_RESET};


void
WirelessInfo::reset()
{
  _ssid = "";
  _channel = -1;
  _bssid = EtherAddress();
  _interval = 100;
  _wep = false;
#if CLICK_NS
  _ifid = -1;
#endif
}
int
WirelessInfo::write_param(const String &in_s, Element *e, void *vparam,
			  ErrorHandler *errh)
{
  WirelessInfo *f = (WirelessInfo *)e;
  String s = cp_uncomment(in_s);
  switch((intptr_t)vparam) {
 case H_SSID: {
    f->_ssid = s;
    break;
  }
  case H_BSSID: {
    EtherAddress e;
    if (!cp_ethernet_address(s, &e))
      return errh->error("bssid parameter must be ethernet address");
    f->_bssid = e;
    break;
  }

  case H_CHANNEL: {
    int m;
    if (!cp_integer(s, &m))
      return errh->error("channel parameter must be int");
    f->_channel = m;
#if CLICK_NS
    if (f->_ifid >= 0)
	simclick_sim_command(f->router()->simnode(), SIMCLICK_CHANGE_CHANNEL, f->_ifid, f->_channel);
#endif
    break;
  }
 case H_INTERVAL: {
    int m;
    if (!cp_integer(s, &m))
      return errh->error("interval parameter must be int");
    f->_interval = m;
    break;
 }
  case H_WEP: {    //debug
    bool wep;
    if (!cp_bool(s, &wep))
      return errh->error("wep parameter must be boolean");
    f->_wep = wep;
    break;
  }
  case H_RESET: f->reset(); break;
  }
  return 0;
}

String
WirelessInfo::read_param(Element *e, void *thunk)
{
  WirelessInfo *td = (WirelessInfo *)e;
    switch ((uintptr_t) thunk) {
    case H_SSID: return td->_ssid + "\n";
    case H_BSSID: return td->_bssid.unparse() + "\n";
    case H_CHANNEL: return String(td->_channel) + "\n";
    case H_INTERVAL: return String(td->_interval) + "\n";
    case H_WEP: return String(td->_wep) + "\n";
    default:
      return "\n";
    }
}

void
WirelessInfo::add_handlers()
{
  add_read_handler("ssid", read_param, (void *) H_SSID);
  add_read_handler("bssid", read_param, (void *) H_BSSID);
  add_read_handler("channel", read_param, (void *) H_CHANNEL);
  add_read_handler("interval", read_param, (void *) H_INTERVAL);
  add_read_handler("wep", read_param, (void *) H_WEP);


  add_write_handler("ssid", write_param, (void *) H_SSID);
  add_write_handler("bssid", write_param, (void *) H_BSSID);
  add_write_handler("channel", write_param, (void *) H_CHANNEL);
  add_write_handler("interval", write_param, (void *) H_INTERVAL);
  add_write_handler("wep", write_param, (void *) H_WEP);
  add_write_handler("reset", write_param, (void *) H_RESET, Handler::BUTTON);

}

CLICK_ENDDECLS
EXPORT_ELEMENT(WirelessInfo)

