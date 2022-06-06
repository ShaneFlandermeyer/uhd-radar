#include "transmit.h"

namespace uhd {
namespace radar {

void transmit(uhd::usrp::multi_usrp::sptr usrp,
              std::vector<std::complex<float> *> buff_ptrs, size_t num_pulses,
              size_t num_samps_pulse, uhd::time_spec_t start_time) {
  static bool first = true;

  static uhd::stream_args_t tx_stream_args("fc32", "sc16");
  static uhd::tx_streamer::sptr tx_stream;
  if (first) {
    tx_stream_args.channels.push_back(0);
    tx_stream = usrp->get_tx_stream(tx_stream_args);
    first = false;
  }

  // Create metadata structure
  static uhd::tx_metadata_t tx_md;
  tx_md.start_of_burst = true;
  tx_md.end_of_burst = false;
  tx_md.has_time_spec = (start_time.get_real_secs() > 0 ? true : false);
  tx_md.time_spec = start_time;

  for (size_t ipulse = 0; ipulse < num_pulses; ipulse++) {
    tx_stream->send(buff_ptrs, num_samps_pulse, tx_md, 0.5);
    tx_md.start_of_burst = false;
    tx_md.has_time_spec = false;
  }

  // Send mini EOB packet
  tx_md.has_time_spec = false;
  tx_md.end_of_burst = true;
  tx_stream->send("", 0, tx_md);
}

}  // namespace radar
}  // namespace uhd