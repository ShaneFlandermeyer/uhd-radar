#include <boost/program_options.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <thread>
#include <chrono>

#include "LinearFMWaveform.h"
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
void transmit(std::vector<std::complex<float>> buffer,
              std::vector<std::complex<float>> waveform,
              uhd::tx_streamer::sptr txStreamer, uhd::tx_metadata_t txMeta,
              size_t step, size_t index, int nChannels) {}

// NOTE: UHD_SAFE_MAIN is just a macro that places a catch-all around the
// regular main()
int main(int argc, char* argv[]) {
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
  float bandwidth = 10e6;
  float pulsewidth = 10e-6;
  float prf = 1e3;
  LinearFMWaveform lfm(bandwidth, pulsewidth, prf, sampleRate);
  auto waveform = lfm.generateWaveform();
  /*
   * Tx streamer setup
   */
  uhd::stream_args_t streamArgs("fc32", "sc16");
  streamArgs.channels = txChanNums;
  uhd::tx_streamer::sptr txStream = usrp->get_tx_stream(streamArgs);
  // Allocate a buffer to store data for each channel
  int spb = txStream->get_max_num_samps() * 10;
  std::vector<std::complex<float>> buff(spb);
  // TODO: When using multiple channels, this should be chan.size()
  int nChan = 1;

  /*
   * Tx metadata
   */
  uhd::tx_metadata_t txMeta;
  txMeta.start_of_burst = true;
  txMeta.end_of_burst = false;
  txMeta.has_time_spec = true;
  // Wait 0.5 seconds as the buffers are filled
  txMeta.time_spec = uhd::time_spec_t(0.5);

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

  // Reset USRP time to prepare for Tx/Rx
  std::cout << boost::format("Setting device timestamp to 0...") << std::endl;
  usrp->set_time_now(0.0);

  // Start Tx thread
  // void transmit(std::vector<std::complex<float>> buffer,
  // std::vector<std::complex<float>> waveform,
  // uhd::tx_streamer::sptr txStreamer, uhd::tx_metadata_t txMeta,
  // size_t step, size_t index, int nChannels)
  // TODO: I really don't think these are necessary
  const size_t step = 1;
  size_t index = 0;
  boost::thread_group txThread;
  txThread.create_thread(
      std::bind(transmit, buff, waveform, txStream, txMeta, step, index,nChan));

  // Transmit for two seconds
  std::this_thread::sleep_for(std::chrono::seconds(2));
  // Clean up transmit worker
  stopSignalCalled = true;
  txThread.join_all();

  return EXIT_SUCCESS;
}
