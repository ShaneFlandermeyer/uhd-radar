#include "receive.h"

namespace uhd {
namespace radar {

void receive(const uhd::rx_streamer::sptr &rx_stream,
             std::vector<std::vector<std::complex<double>>> &rx_data,
             size_t num_samps_to_receive, double start_time) {
  // Create rx buffers/pointers from rx data
  std::vector<std::complex<double> *> rx_buffers(rx_data.size());
  for (size_t i_chan = 0; i_chan < rx_data.size(); i_chan++)
    rx_buffers[i_chan] = rx_data[i_chan].data();
  size_t num_samps_rx_buffer = rx_stream->get_max_num_samps();

  // Set up the stream command object
  // TODO: Add support for continuous streaming
  uhd::stream_cmd_t stream_cmd(
      uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
  stream_cmd.num_samps = num_samps_to_receive;
  stream_cmd.stream_now = false;
  stream_cmd.time_spec = uhd::time_spec_t(start_time);
  rx_stream->issue_stream_cmd(stream_cmd);
  uhd::rx_metadata_t rx_meta;

  // Main receive loop
  double timeout = start_time + 0.1;
  std::vector<std::complex<double> *> curr_buff_offset(rx_buffers.size());
  size_t num_samps_received = 0;
  while (num_samps_received < num_samps_to_receive) {
    // Get pointers to the current buffer offsets for each channel
    for (size_t i_chan = 0; i_chan < rx_buffers.size(); i_chan++)
      curr_buff_offset[i_chan] = rx_buffers[i_chan] + num_samps_received;
    // Determine the # of samples to receive in this call to recv() then do it
    size_t num_samps_to_receive_this_iter = std::min(
        num_samps_to_receive - num_samps_received, num_samps_rx_buffer);
    num_samps_received += rx_stream->recv(
        curr_buff_offset, num_samps_to_receive_this_iter, rx_meta, timeout);
    // After the first call, the timeout can be set to a smaller value
    timeout = 0.1;

    // Check for errors after every packet
    handle_receive_errors(rx_meta);
  }
  stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
  rx_stream->issue_stream_cmd(stream_cmd);
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