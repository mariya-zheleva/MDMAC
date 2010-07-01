/*
 * This element is generated for the needs of MDMAC. It's purpose
 * is to serve as ACK generator and to be able to read incoming 
 * packet's headers and generate the corresponding ACK BEFORE
 * wifi decapsulation.
 */

#include <click/config.h>
#include <click/glue.hh>
#include <clicknet/wifi.h>
#include <click/confparse.hh>
#include <click/packet.hh>
#include "mdmacackresponder.hh"
CLICK_DECLS

MdmacACKResponder::MdmacACKResponder()
{
}

MdmacACKResponder::~MdmacACKResponder()
{
}

Packet *
MdmacACKResponder::simple_action(Packet *p)
{
    struct click_wifi *w = (struct click_wifi *) p->data(); // Received the wifi packet
    EtherAddress dest(w->i_addr1); // Get the destination address from the packet
    if (dest == _eth) { // If destination is own MAC address
      WritablePacket *xp = Packet::make(sizeof(click_wifi)); // Make an object of type WritablePacket with the size of wifi packet 
      //???//xp->pull(2); // Remove headers.
      click_wifi *wifi = (click_wifi *) xp->data(); // create a pointer to writablePacket xp
      //      w->i_fc[0] = (uint8_t) ((WIFI_FC0_VERSION_0 | WIFI_FC0_TYPE_DATA) | WIFI_FC0_SUBTYPE_MDMAC); // wifi packet type. For MDMAC ACK is 1f
      memcpy(wifi->i_addr2, w->i_addr1, 6);  // dst = i_addr1  src = i_addr2  bssid = i_addr3
      memcpy(wifi->i_addr1, w->i_addr2, 6);
      memcpy(wifi->i_seq, w->i_seq, 2); //copy the seq number
      xp->timestamp_anno().set_now();
      output(1).push(xp);
    }
    return p;
}

int
MdmacACKResponder::configure(Vector<String> &conf, ErrorHandler *errh)
{
    return cp_va_kparse(conf, this, errh,
            "ETH", cpkP+cpkM, cpEthernetAddress, &_eth,
            cpEnd);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(MdmacACKResponder)

