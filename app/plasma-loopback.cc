#include <plasma-dsp/linear-fm-waveform.h>

#include <boost/thread.hpp>
#include <iostream>
#include <memory>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <vector>

/**
 * @brief Transmit the pulsed waveform
 *
 * @param waveform
 * @param buffers
 * @param tx_streamer
 * @param start_time
 * @param num_samps_tx
 */
void transmit(plasma::PulsedWaveform &waveform,
              std::vector<std::complex<double> *> buffers,
              uhd::tx_streamer::sptr tx_stream, double start_time = 0.1,
              int num_samps_to_send = 0) {
  // TODO: Multi-prf support
  double pri = 1 / waveform.prf()[0];
  double send_time = start_time;
  double timeout = pri + 0.1;

  // Metadata
  uhd::tx_metadata_t tx_meta;
  tx_meta.has_time_spec = true;
  tx_meta.time_spec = uhd::time_spec_t(send_time);
  tx_meta.start_of_burst = true;
  tx_meta.end_of_burst = false;
  // TODO: Be smarter about this number
  std::vector<std::complex<double>> data = waveform.pulse();
  size_t num_samps_pulse = data.size();
  size_t num_samps_sent = 0;
  while (num_samps_sent < num_samps_to_send or num_samps_to_send == 0) {
    num_samps_sent += tx_stream->send(buffers, num_samps_pulse, tx_meta);
    tx_meta.has_time_spec = true;
    send_time += pri;
    tx_meta.time_spec = uhd::time_spec_t(send_time);
  }
  // Send a mini EOB packet
  tx_meta.end_of_burst = true;
  tx_stream->send("", 0, tx_meta);
}

void receive(std::vector<std::complex<double> *> buffers,
             uhd::rx_streamer::sptr rx_stream, double start_time = 0.1,
             int num_samps_to_receive = 0) {
  uhd::stream_cmd_t stream_cmd(
      (num_samps_to_receive == 0)
          ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
          : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
  stream_cmd.num_samps = num_samps_to_receive;
  stream_cmd.stream_now = false;
  stream_cmd.time_spec = uhd::time_spec_t(start_time);
  rx_stream->issue_stream_cmd(stream_cmd);
  // Create metadata object
  uhd::rx_metadata_t rx_meta;
  size_t num_samps_rx_buffer = rx_stream->get_max_num_samps();
  // double timeout = start_time + 0.1;
  size_t num_samps_received = 0;
  while (num_samps_received < num_samps_to_receive or
         num_samps_to_receive == 0) {
    num_samps_received +=
        rx_stream->recv(buffers, num_samps_rx_buffer, rx_meta);
    // Error handling
    if (rx_meta.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
      std::cout << boost::format("Timeout while streaming") << std::endl;
      break;
    }
    if (rx_meta.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
      throw std::runtime_error(
          str(boost::format("Receiver error %s") % rx_meta.strerror()));
    }
  }
  stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
  rx_stream->issue_stream_cmd(stream_cmd);
}
/**
 * @brief Check for LO lock
 *
 * TODO: These types of helper functions should be moved to a uhd-radar
 * library
 *
 * @param usrp multi_usrp sptr
 */
void CheckLoLock(uhd::usrp::multi_usrp::sptr usrp) {
  std::vector<std::string> txSensorNames, rxSensorNames;
  txSensorNames = usrp->get_tx_sensor_names(0);
  rxSensorNames = usrp->get_rx_sensor_names(0);
  // Tx check
  if (std::find(txSensorNames.begin(), txSensorNames.end(), "lo_locked") !=
      txSensorNames.end()) {
    uhd::sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked", 0);
    std::cout << boost::format("Checking Tx %s") % lo_locked.to_pp_string()
              << std::endl;
    UHD_ASSERT_THROW(lo_locked.to_bool());
  }
  // Rx check
  if (std::find(rxSensorNames.begin(), rxSensorNames.end(), "lo_locked") !=
      rxSensorNames.end()) {
    uhd::sensor_value_t lo_locked = usrp->get_rx_sensor("lo_locked", 0);
    std::cout << boost::format("Checking Rx %s") % lo_locked.to_pp_string()
              << std::endl;
    UHD_ASSERT_THROW(lo_locked.to_bool());
  }
}

int UHD_SAFE_MAIN(int argc, char *argv[]) {
  // USRP setup
  std::string usrp_args = "";
  double samp_rate = 20e6;
  double center_freq = 5e9;
  double tx_gain = 50;
  double rx_gain = 50;
  std::vector<size_t> tx_chan_nums(1, 0);
  auto usrp = uhd::usrp::multi_usrp::make(usrp_args);
  usrp->set_tx_rate(samp_rate);
  usrp->set_rx_rate(samp_rate);
  usrp->set_tx_freq(center_freq);
  usrp->set_rx_freq(center_freq);
  usrp->set_tx_gain(tx_gain);
  usrp->set_rx_gain(rx_gain);
  CheckLoLock(usrp);

  // Waveform setup
  double bandwidth = samp_rate / 2;
  double pulsewidth = 10e-6;
  size_t num_samps_pulse = std::round(samp_rate * pulsewidth);
  double prf = 1e3;
  double pri = 1 / prf;
  plasma::LinearFMWaveform wave(bandwidth, pulsewidth, prf, samp_rate);
  std::vector<std::complex<double>> waveform = wave.pulse();

  // Stream objects
  std::string cpu_format = "fc64";
  std::string otw_format = "sc16";
  uhd::stream_args_t stream_args(cpu_format, otw_format);
  stream_args.channels = tx_chan_nums;
  uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
  uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

  // Tx buffer initialization
  size_t num_samps_tx_buffer = waveform.size();
  // TODO: No idea why get_tx_num_channels returns 2 here
  size_t num_tx_chan = usrp->get_tx_num_channels();
  std::vector<std::vector<std::complex<double>>> tx_buffers(
      num_tx_chan, std::vector<std::complex<double>>(num_samps_tx_buffer));
  // TODO: Only using one buffer/waveform for now
  std::copy(waveform.begin(), waveform.end(), tx_buffers[0].begin());
  std::vector<std::complex<double> *> tx_buffer_pointers(num_tx_chan);
  for (size_t i = 0; i < num_tx_chan; i++) {
    tx_buffer_pointers[i] = &tx_buffers[i].front();
  }

  // Rx buffer initialization
  size_t num_samps_rx_buffer = rx_stream->get_max_num_samps();
  size_t num_rx_chan = usrp->get_rx_num_channels();
  std::vector<std::vector<std::complex<double>>> rx_buffers(
      num_rx_chan, std::vector<std::complex<double>>(num_samps_rx_buffer));
  std::vector<std::complex<double> *> rx_buffer_pointers(num_rx_chan);
  for (size_t i = 0; i < num_rx_chan; i++) {
    rx_buffer_pointers[i] = &rx_buffers[i].front();
  }

  // Set a nonzero start time to avoid an initial transient
  double start_time = 0.1;

  double duration = 3;
  size_t num_samps_total = std::round(samp_rate * duration);
  size_t num_rx_offset_samps = 0;
  usrp->set_time_now(0.0);
  boost::thread_group tx_thread, rx_thread;
  // transmit(wave, tx_buffer_pointers, tx_stream, start_time,
  // num_samps_total);
  tx_thread.create_thread(std::bind(&transmit, wave, tx_buffer_pointers,
                                    tx_stream, start_time, num_samps_total));
  rx_thread.create_thread(std::bind(&receive, rx_buffer_pointers, rx_stream,
                                    start_time, num_samps_total));
  tx_thread.join_all();
  rx_thread.join_all();

  return EXIT_SUCCESS;
}
