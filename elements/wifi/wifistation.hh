#ifndef CLICK_WIFISTATION_HH
#define CLICK_WIFISTATION_HH
#include <click/element.hh>
#include <clicknet/ether.h>
CLICK_DECLS

/*
=c

WifiStation(CHANNEL)

=s decapsulation, Wifi -> Ethernet

Turns 80211 packets into ethernet packets encapsulates packets in Ethernet header

=d


If channel is 0, it doesn't filter any beacons.
If channel is < 0, it doesn't look at any beconds
if channel is > 0, it looks at only beacons with on channel.
=e


  wifi_cl :: Classifier (0/00%0c, 
                         0/04%0c,
                         0/08%0c);

  wifi_cl [0] -> Discard; //mgt 
  wifi_cl [1] -> Discard; //ctl
  wifi_cl [2] -> wifi_decap :: WifiStation() -> ...

=a

EtherEncap */

class WifiStation : public Element { public:
  
  WifiStation();
  ~WifiStation();

  const char *class_name() const	{ return "WifiStation"; }
  const char *processing() const	{ return PUSH; }
  
  int configure(Vector<String> &, ErrorHandler *);
  bool can_live_reconfigure() const	{ return true; }

  void push(int, Packet *);
  void send_assoc_req();

  void add_handlers();
  void reset();

  bool _debug;

  EtherAddress _eth;
  EtherAddress _bssid;
  String _ssid;
  uint16_t _listen_interval;
  Vector<int> _rates;

  String scan_string();
 private:



};

CLICK_ENDDECLS
#endif