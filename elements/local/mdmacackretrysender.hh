#ifndef CLICK_MDMACACKRETRYSENDER_HH
#define CLICK_MDMACACKRETRYSENDER_HH
#include <click/element.hh>
#include <click/etheraddress.hh>
#include <click/packet.hh>
#include <click/task.hh>
#include <click/timer.hh>
#include <click/dequeue.hh>
CLICK_DECLS

/*
 * =c
 * MdmacACKRetrySender(I<KEYWORDS>)
 *
 * =s Grid
 * Resend packets until a positive acknowledgement is received.
 *
 * =d
 *
 * Input 0 should be Ethernet packets.  Input 1 should be
 * acknowledgements.  When a packet is pulled in on input 0, it is
 * pushed on output 0, and cached until a positive acknowledgement
 * (ACK) is received.  If no ACK is received before the resend timer
 * expires, the packet is resent.  If the packet has been resent too
 * many times, it is pushed to output 1.  If output 1 is not
 * connected, it is dropped.
 *
 * Keyword arguments are:
 *
 * =over 8
 *
 * =item MAX_TRIES
 *
 * Unsigned integer, > 0.  Send the packet up to this many times
 * before giving up.  Default is 16.  This includes the initial
 * transmission.
 *
 * =item TIMEOUT
 *
 * Unsigned integer, > 0.  Milliseconds.  Wait this long before
 * resending the packet.  Default is 10.
 *
 * =item HISTORY_SZ
 *
 * Unsigned integer.  Number of most recent packets for which to
 * remember retry data.  Defaults to 500.
 *
 * =item VERBOSE
 *
 * Boolean.  Be noisy.  True by default.
 *
 * =back
 *
 * =h summary read-only
 * Print summary of packet retry statistics
 *
 * =h history read-only
 * Print packet retry history.
 *
 * =h clear write-only
 * Clear out packet retry history.
 *
 * =a
 * ACKResponder, MdmacACKRetrySender2, ACKResponder2 */

class MdmacACKRetrySender : public Element {
public:
  MdmacACKRetrySender();
  ~MdmacACKRetrySender();

  const char *class_name() const { return "MdmacACKRetrySender"; }
  const char *port_count() const { return "-/-"; }
  const char *processing() const { return "la/hh"; }
  const char *flow_code()  const { return "xy/xx"; }

  int configure(Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *errh);

  bool run_task(Task *);
  void run_timer(Timer *);
  void push(int, Packet *);
  void printarray (char *msg, uint8_t arg[], int length);

  void add_handlers();

private:
  unsigned int _timeout; // msecs
  unsigned int _max_tries; // max number of times to tx before quitting
  unsigned int _num_tries; // number of times current packet has been sent
  unsigned int _history_length;
  bool _cansend;

  struct tx_result_t {
    tx_result_t(const Timestamp &t, unsigned n, bool s)
      : pkt_time(t), num_tx(n), success(s) { }
    Timestamp pkt_time;
    unsigned num_tx;
    bool success;
  };

  typedef DEQueue<tx_result_t> HistQ;
  HistQ _history;

  Packet *_waiting_packet;
  bool _verbose;

  Timer _timer;
  Task _task;

  void check();

  static String print_history(Element *e, void *);
  static String print_summary(Element *e, void *);
  static int clear_history(const String &, Element *, void *, ErrorHandler *);
};

CLICK_ENDDECLS
#endif
