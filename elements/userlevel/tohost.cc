// -*- c-basic-offset: 4 -*-
/*
 * tohost.{cc,hh} -- element sends packets to Linux through the 
 * TUN Universal TUN/TAP module
 * John Bicket
 *
 * Copyright (c) 2004 Massachusetts Institute of Technology
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
#include "tohost.hh"
#include <click/error.hh>
#include <click/bitvector.hh>
#include <click/confparse.hh>
#include <click/straccum.hh>
#include <click/glue.hh>
#include <clicknet/ether.h>
#include <click/standard/scheduleinfo.hh>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <net/if.h>
#include <linux/if_tun.h>
#include <click/router.hh>
#include "fromhost.hh"

CLICK_DECLS

ToHost::ToHost()
  : Element(1, 0), 
    _fd(-1)
{
  MOD_INC_USE_COUNT;
}

ToHost::~ToHost()
{
  MOD_DEC_USE_COUNT;
}

int
ToHost::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (cp_va_parse(conf, this, errh,
		  cpString, "device name", &_dev_name, 
		  cpOptional,
		  cpKeywords,
		  cpEnd) < 0)
    return -1;

  return 0;
}


int
ToHost::initialize(ErrorHandler *errh)
{
  //find a FromHost and reuse its socket
    for (int ei = 0; ei < router()->nelements() && _fd < 0; ei++) {
	Element *e = router()->element(ei);
	FromHost *s = (FromHost *)e->cast("FromHost");
	if (s && 
	    s->dev_name() == _dev_name && 
	    s->fd() > 0) {
	    _fd = s->fd();
	    return 0;
	}
    }
    
    return errh->error("ToHost(%s) requires an initialized FromHost with the same dev_name",
		       _dev_name.cc());
}

void
ToHost::push(int, Packet *p)
{
    if (p->length() < sizeof(struct click_ether)){
	click_chatter("ToHost: packet to small");
	p->kill();
	return;
    }

    // 2-byte padding followed by an Ethernet type
    WritablePacket *q = p->push(4);
    if (q) {
	click_ether *e = (click_ether *) (p->data() + 4);
	*(uint32_t *)(q->data()) = e->ether_type;

	int w = write(_fd, q->data(), q->length());
	static bool _printed_write_err = false;
	if (w != (int) q->length() && (errno != ENOBUFS || !_printed_write_err)) {
	    _printed_write_err = true;
	    click_chatter("KernelTun(%s): write failed: %s", _dev_name.cc(), strerror(errno));
	}
	q->kill();
    } else
	click_chatter("%{element}: out of memory", this);
}


enum {H_DEV_NAME};

static String 
ToHost_read_param(Element *e, void *thunk)
{
  ToHost *td = (ToHost *)e;
    switch ((uintptr_t) thunk) {
    case H_DEV_NAME:
    return td->dev_name() + "\n";
    default:
	return "\n";
    }
}



void
ToHost::add_handlers()
{
  add_read_handler("dev_name", ToHost_read_param, (void *) H_DEV_NAME);
}

CLICK_ENDDECLS
ELEMENT_REQUIRES(userlevel FromHost)
EXPORT_ELEMENT(ToHost)