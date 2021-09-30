#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include <csignal>
#include <thread>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>

#include "LinearFMWaveform.h"

#define VERBOSE true
#define STOP_TIME 5
/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stopSignalCalled = false;
void sigIntHandler(int) { stopSignalCalled = true; }

namespace po = boost::program_options;

/*
 * Worker function to handle transmit operation
 * (Gets its own thread in main())
 */
void transmit(LinearFMWaveform& waveform,
              std::vector<std::complex<float>*> buffs,
              uhd::tx_streamer::sptr txStream) {
  double sendTime = 0.1;
  uhd::tx_metadata_t txMeta;
  txMeta.has_time_spec = true;
  txMeta.time_spec = uhd::time_spec_t(sendTime);
  int nSampsPulse = std::round(waveform.sampleRate / waveform.prf);
  int pri = 1 / waveform.prf;
  size_t nTxSamps = txStream->send(buffs, nSampsPulse, txMeta, sendTime);
  while (!stopSignalCalled) {
    sendTime += pri;
    txMeta.has_time_spec = true;
    txMeta.time_spec = uhd::time_spec_t(sendTime);
    nTxSamps = txStream->send(buffs, nSampsPulse, txMeta, sendTime);
  }
}

// NOTE: UHD_SAFE_MAIN is just a macro that places a catch-all around the
// regular main()
int main(int argc, char* argv[]) {
  std::signal(SIGINT, &sigIntHandler);
  /*
   * USRP device setup
   */
  std::string usrpArgs = "";
  double sampleRate = 20e6;
  double centerFreq = 5e9;
  double txGain = 50;
  double rxGain = 50;
  std::vector<size_t> txChanNums(1, 0);

  uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(usrpArgs);
  // Set the sample rate
  usrp->set_tx_rate(sampleRate);
  usrp->set_rx_rate(sampleRate);
  // Set center frequency
  usrp->set_tx_freq(centerFreq);
  usrp->set_rx_freq(centerFreq);
  // Set RF gain
  usrp->set_tx_gain(txGain);
  usrp->set_rx_gain(rxGain);

  /*
   * Waveform setup
   */
  float bandwidth = 20e6;
  float pulsewidth = 10e-6;
  double prf = 10e3;
  double pri = 1 / prf;
  LinearFMWaveform lfm(bandwidth, pulsewidth, prf, sampleRate);

  /*
   * Check for ref & LO lock
   */
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

  uhd::stream_args_t streamArgs("fc32", "sc16");
  streamArgs.channels = txChanNums;
  uhd::tx_streamer::sptr txStream = usrp->get_tx_stream(streamArgs);
  bool repeat = true;
  auto waveform = lfm.generateWaveform();
  int nSampsPulse = std::round(sampleRate*pulsewidth);
  auto buff = waveform;
  // Create a vector of zeros
  std::vector<std::complex<float>*> buffs(1, &buff.front());
  usrp->set_time_now(0.0);
  boost::thread_group txThread;
  txThread.create_thread(std::bind(&transmit, lfm,buffs, txStream));

  txThread.join_all();

  std::cout << "Done (with failure!)" << std::endl;
  return EXIT_SUCCESS;
}
