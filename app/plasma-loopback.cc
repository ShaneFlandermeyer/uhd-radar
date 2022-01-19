#include <matplot/matplot.h>
#include <plasma-dsp/linear-fm-waveform.h>

#include <boost/thread.hpp>
#include <iostream>
#include <memory>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <vector>

#include "receive.h"
#include "transmit.h"

using namespace matplot;

inline void HandleReceiveErrors(const uhd::rx_metadata_t &rx_meta) {
  if (rx_meta.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
    std::cout << boost::format("Timeout while streaming") << std::endl;
    return;
  }
  if (rx_meta.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
    throw std::runtime_error(
        str(boost::format("Receiver error %s") % rx_meta.strerror()));
  }
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
  std::string usrp_args = "serial=315EED9";
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
  double pulsewidth = 100e-6;
  Eigen::ArrayXd prf(2);
  prf << 1e3, 2e3;
  size_t num_samps_pulse = round(samp_rate * pulsewidth);
  
  // size_t num_samps_pri = round(samp_rate / prf(0));
  plasma::LinearFMWaveform wave(bandwidth, pulsewidth, prf, samp_rate);
  Eigen::ArrayXcd pulse = wave.waveform();
  std::vector<std::complex<double>> waveform(pulse.data(),
                                             pulse.data() + pulse.size());
  // Stream objects
  std::string cpu_format = "fc64";
  std::string otw_format = "sc16";
  uhd::stream_args_t stream_args(cpu_format, otw_format);
  stream_args.channels = tx_chan_nums;
  uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
  uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

  // Tx buffer initialization
  size_t num_samps_tx_buffer = waveform.size();
  // TODO: Add multi-channel support
  size_t num_tx_chan = 1;
  std::vector<std::vector<std::complex<double>>> tx_buffers(
      num_tx_chan, std::vector<std::complex<double>>(num_samps_tx_buffer));
  std::copy(waveform.begin(), waveform.end(), tx_buffers[0].begin());

  // Set a nonzero start time to avoid an initial transient
  double start_time = 0.5;
  size_t num_pulses = 10;
  
  // Compute the total number of samples in the collect
  Eigen::ArrayXi num_samps_pri = round(samp_rate / prf).cast<int>();
  size_t num_samps_total = 0;
  for (size_t i = 0; i < num_pulses; i++) {
    num_samps_total += num_samps_pri(i % prf.size());
  }

  // Rx buffer initialization
  // TODO: Add multi-channel support
  size_t num_rx_chan = 1;
  std::vector<std::vector<std::complex<double>>> rx_buffers(
      num_rx_chan, std::vector<std::complex<double>>(num_samps_total));

  usrp->set_time_now(0.0);
  boost::thread_group tx_thread, rx_thread, write_thread;
  tx_thread.create_thread(
      [&tx_stream, &tx_buffers, &prf, &num_pulses, &start_time]() {
        uhd::radar::transmit(
            tx_stream, tx_buffers,
            std::vector<double>(prf.data(), prf.data() + prf.size()),
            num_pulses, start_time);
      });
  // TODO: Add receive support for multi-prf
  rx_thread.create_thread(
      [&rx_stream, &rx_buffers, &num_samps_total, &start_time]() {
        uhd::radar::receive(rx_stream, rx_buffers, num_samps_total, start_time);
      });
  tx_thread.join_all();
  rx_thread.join_all();

  size_t num_rx_offset_samps = 164;
  rx_buffers[0].erase(rx_buffers[0].begin(),
                      rx_buffers[0].begin() + num_rx_offset_samps);

  figure();
  plot(plasma::real(rx_buffers[0]));
  // hold(true);
  // plot(plasma::imag(rx_buffers[0]));
  // hold(false);
  show();

  return EXIT_SUCCESS;
}
