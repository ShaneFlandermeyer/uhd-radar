#include "transmit.h"

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

void transmit(uhd::tx_streamer::sptr tx_stream,
              std::vector<std::vector<std::complex<double>>> tx_data,
              std::vector<double> prf_schedule, size_t num_pulses_to_send,
              double start_time) {
  // Create tx buffers/pointers from tx data
  size_t num_tx_chan = tx_data.size();
  // TODO: This should be an array
  // The pulse is not necessarily the same size for every channel
  size_t num_samps_pulse = tx_data[0].size();
  std::vector<std::complex<double> *> tx_buffers(num_tx_chan);
  for (size_t i_chan = 0; i_chan < num_tx_chan; i_chan++)
    tx_buffers[i_chan] = tx_data[i_chan].data();

  // Get the PRI corresponding to each PRF in the schedule
  std::vector<double> pri_schedule(prf_schedule.size());
  for (size_t i_prf = 0; i_prf < prf_schedule.size(); i_prf++)
    pri_schedule[i_prf] = 1 / prf_schedule[i_prf];
  size_t pri_index = 0;

  uhd::tx_metadata_t tx_meta;
  double send_time = start_time;
  double timeout = std::max(start_time, pri_schedule[pri_index]) + 0.1;
  size_t num_pulses_sent = 0;
  while (num_pulses_sent < num_pulses_to_send) {
    // Transmit the pulse as a single UHD burst
    tx_meta.start_of_burst = true;
    tx_meta.has_time_spec = true;
    tx_meta.time_spec = uhd::time_spec_t(send_time);
    tx_stream->send(tx_buffers, num_samps_pulse, tx_meta, timeout);

    // Send an empty packet to signal the end of the burst
    tx_meta.start_of_burst = false;
    tx_meta.end_of_burst = true;
    tx_meta.has_time_spec = false;
    tx_stream->send("", 0, tx_meta);

    // Update counters
    send_time += pri_schedule[pri_index];
    num_pulses_sent++;
    pri_index = num_pulses_sent % pri_schedule.size();
    timeout = pri_schedule[pri_index];
    
  }
}
}  // namespace radar
}  // namespace uhd