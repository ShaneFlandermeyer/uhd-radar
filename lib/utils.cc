#include "utils.h"
namespace test {
void transmit(uhd::tx_streamer::sptr tx_stream,
              std::vector<std::vector<std::complex<double>>> tx_data,
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
}  // namespace test