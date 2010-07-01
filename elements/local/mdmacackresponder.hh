#ifndef CLICK_MDMACACKRESPONDER_HH
#define CLICK_MDMACACKRESPONDER_HH
#include <click/element.hh>
#include <click/etheraddress.hh>
CLICK_DECLS                        

#define ETHERTYPE_MDMAC_ACK 0x7ffb 

class MdmacACKResponder : public Element {
public:
  MdmacACKResponder();
  ~MdmacACKResponder();

  const char *class_name() const { return "MdmacACKResponder"; }
  const char *port_count() const { return "1/2"; }
  const char *processing() const { return PROCESSING_A_AH; }
  const char *flow_code()  const { return "x/xy"; }

  int configure(Vector<String> &, ErrorHandler *);

  Packet *simple_action(Packet *);

private:
  EtherAddress _eth;
};

CLICK_ENDDECLS
#endif

