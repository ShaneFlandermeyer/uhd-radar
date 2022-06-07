#include "receive.h"

namespace uhd {
namespace radar {

long int receive(uhd::usrp::multi_usrp::sptr usrp,
                 std::vector<std::complex<float> *> buff_ptrs, size_t num_samps,
                 uhd::time_spec_t start_time) {
  // TODO: Beware a bug on X3xx radios where recreation of streams will fail
  // after 256 iterations. It might be necessary to make the streamer static so
  // it is only created once. See
  // https://marc.info/?l=usrp-users&m=145047669625107&w=1 for bug info, and
  // https://github.com/jonasmc83/USRP_Software_defined_radar for a possible
  // solution

  static bool first = true;

  static size_t channels = buff_ptrs.size();
  static std::vector<size_t> channel_vec;
  static uhd::stream_args_t stream_args("fc32", "sc16");
  uhd::rx_streamer::sptr rx_stream;
  // rx_stream.reset();
  if (first) {
    first = false;
    if (channel_vec.size() == 0) {
      for (size_t i = 0; i < channels; i++) {
        channel_vec.push_back(i);
      }
    }
    stream_args.channels = channel_vec;
    // rx_stream = usrp->get_rx_stream(stream_args);
  }
  rx_stream = usrp->get_rx_stream(stream_args);

  uhd::rx_metadata_t md;

  // Set up streaming
  uhd::stream_cmd_t stream_cmd(
      uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
  stream_cmd.num_samps = num_samps;
  stream_cmd.time_spec = start_time;
  stream_cmd.stream_now = (start_time.get_real_secs() > 0 ? false : true);
  rx_stream->issue_stream_cmd(stream_cmd);

  size_t max_num_samps = rx_stream->get_max_num_samps();
  size_t num_samps_total = 0;
  std::vector<std::complex<float> *> buff_ptrs2(buff_ptrs.size());
  double timeout = 0.5 + start_time.get_real_secs();
  while (num_samps_total < num_samps) {
    // Move storing pointer to correct location
    for (size_t i = 0; i < channels; i++)
      buff_ptrs2[i] = &(buff_ptrs[i][num_samps_total]);

    // Sampling data
    size_t samps_to_recv = std::min(num_samps - num_samps_total, max_num_samps);
    size_t num_rx_samps =
        rx_stream->recv(buff_ptrs2, samps_to_recv, md, timeout);
    timeout = 0.5;

    num_samps_total += num_rx_samps;

    // handle the error code
    if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) break;
    if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) break;
  }

  if (num_samps_total < num_samps)
    return -((long int)num_samps_total);
  else
    return (long int)num_samps_total;
}

inline void handle_receive_errors(const uhd::rx_metadata_t &rx_meta) {
  if (rx_meta.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
    std::cout << boost::format("No packet received, implementation timed-out.")
              << std::endl;
    return;
  }
  if (rx_meta.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
    throw std::runtime_error(
        str(boost::format("Receiver error %s") % rx_meta.strerror()));
  }
}

}  // namespace radar
}  // namespace uhd