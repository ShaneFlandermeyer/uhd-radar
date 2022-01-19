#include "utils.h"
namespace uhd {
namespace radar {
void transmit(const uhd::tx_streamer::sptr &tx_stream,
              std::vector<std::vector<std::complex<double>>> &tx_data,
              double prf, size_t num_pulses_to_send, double start_time = 0.1) {
  // Create tx buffers/pointers from tx data
  size_t num_tx_chan = tx_data.size();
  // TODO: This should be an array
  // The pulse is not necessarily the same size for every channel
  size_t num_samps_pulse = tx_data[0].size();
  std::vector<std::complex<double> *> tx_buffers(num_tx_chan);
  for (size_t i_chan = 0; i_chan < num_tx_chan; i_chan++)
    tx_buffers[i_chan] = tx_data[i_chan].data();

  double pri = 1 / prf;
  uhd::tx_metadata_t tx_meta;
  double send_time = start_time;
  size_t num_pulses_sent = 0;
  double timeout = std::max(start_time, pri) + 0.1;
  while (num_pulses_sent < num_pulses_to_send) {
    // Transmit the pulse as a single UHD burst
    tx_meta.start_of_burst = true;
    tx_meta.has_time_spec = true;
    tx_meta.time_spec = uhd::time_spec_t(send_time);

    tx_stream->send(tx_buffers, num_samps_pulse, tx_meta, timeout);
    timeout = pri;

    // Send an empty packet to signal the end of the burst
    tx_meta.start_of_burst = false;
    tx_meta.end_of_burst = true;
    tx_meta.has_time_spec = false;
    tx_stream->send("", 0, tx_meta);

    // Update counters
    send_time += pri;
    num_pulses_sent++;
  }
}

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

    // TODO: Check for errors
  }
  stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
  rx_stream->issue_stream_cmd(stream_cmd);
}

}  // namespace radar
}  // namespace uhd