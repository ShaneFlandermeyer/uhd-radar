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
 * Check for LO lock
 */
void checkLoLock(uhd::usrp::multi_usrp::sptr usrp) {
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

/*
 * Worker function to handle transmit operation
 */
void transmit(LinearFMWaveform &waveform,
              std::vector<std::complex<float> *> buffs,
              uhd::tx_streamer::sptr txStream, double startTime = 0.1) {
  // Pulse repetition interval
  double pri = 1 / waveform.prf;
  // Time spec for the first packet
  double sendTime = startTime;
  // Timeout for send(). If this is reached on transmit, we have done something
  // wrong
  double timeout = 0.1 + pri;
  // Set up the Tx metadata
  uhd::tx_metadata_t txMeta;
  txMeta.has_time_spec = true;
  txMeta.time_spec = uhd::time_spec_t(sendTime);
  txMeta.start_of_burst = true;
  txMeta.end_of_burst = false;
  // Every call to send() will transmit an entire PRI
  // TODO: Make this bursty so we don't have to explicitly transmit zeros
  int nSampsPulse = std::round(waveform.sampleRate / waveform.prf);
  size_t nTxSamps;
  // TODO: Add a time condition to tell the function when to exit
  while (not stopSignalCalled) {
    nTxSamps = txStream->send(buffs, nSampsPulse, txMeta, timeout);
    // Update metadata
    sendTime += pri;
    txMeta.has_time_spec = true;
    txMeta.time_spec = uhd::time_spec_t(sendTime);
    txMeta.start_of_burst = false;
  }
  // Send a mini EOB packet
  txMeta.end_of_burst = true;
  txStream->send("",0,txMeta);
}

/*
 * Worker function to handle receive operation
 */
void receive(std::vector<std::complex<float> *> buffs,
             uhd::rx_streamer::sptr rxStream, double startTime = 0.1,
             int nSampsRequested = 0) {
  // Send a stream command to tell the rx stream when to start
  uhd::stream_cmd_t streamCmd(
      (nSampsRequested == 0)
          ? uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS
          : uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
  streamCmd.num_samps = nSampsRequested;
  streamCmd.stream_now = false;
  streamCmd.time_spec = uhd::time_spec_t(startTime);
  rxStream->issue_stream_cmd(streamCmd);

  // Create metadata object
  uhd::rx_metadata_t rxMeta;
  const size_t nSampsRxBuff = rxStream->get_max_num_samps();
  // Timeout value for recv()
  // TODO: Make this a parameter
  double timeout = 0.1;
  int nSampsTotal = 0;
  // TODO: Add a time condition to tell the function when to exit
  while (not stopSignalCalled and
         (nSampsTotal < nSampsRequested or nSampsRequested == 0)) {
    size_t nSampsRx = rxStream->recv(buffs, nSampsRxBuff, rxMeta, timeout);
    // Check for errors
    if (rxMeta.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
      std::cout << boost::format("Timeout while streaming") << std::endl;
      break;
    }
    if (rxMeta.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
      throw std::runtime_error(
          str(boost::format("Receiver error %s") % rxMeta.strerror()));
    }
    // Update the sample count
    nSampsTotal += nSampsRx;

    // TODO: Write to file
  }
  // Shut down the receiver
  streamCmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
  rxStream->issue_stream_cmd(streamCmd);

  // TODO: Close files
}

/**************************************************************************
 * Main function
 ***********************************************************************/
int UHD_SAFE_MAIN(int argc, char *argv[]) {
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
  int nSampsPulse = std::round(sampleRate * pulsewidth);
  double prf = 10e3;
  double pri = 1 / prf;
  LinearFMWaveform lfm(bandwidth, pulsewidth, prf, sampleRate);
  auto waveform = lfm.generateWaveform();

  // Check LO lock before continuing
  checkLoLock(usrp);

  /*
   * Create Streamer objects
   */
  uhd::stream_args_t streamArgs("fc32", "sc16");
  streamArgs.channels = txChanNums;
  uhd::tx_streamer::sptr txStream = usrp->get_tx_stream(streamArgs);
  uhd::rx_streamer::sptr rxStream = usrp->get_rx_stream(streamArgs);

  /*
   * Initialize buffers
   */
  // Initialize the Tx buffer(s)
  const size_t nSampsTxBuff = waveform.size();
  int nTxChan = usrp->get_tx_num_channels();
  std::vector<std::vector<std::complex<float>>> txBuffs(
      nTxChan, std::vector<std::complex<float>>(nSampsTxBuff));
  // TODO: Only using one waveform and one channel for now
  for (int i = 0; i < nSampsTxBuff; i++) txBuffs[0][i] = waveform[i];
  std::vector<std::complex<float> *> txBuffPtrs;
  for (size_t i = 0; i < nTxChan; i++)
    txBuffPtrs.push_back(&txBuffs[i].front());
  // Initialize the Rx buffer(s)
  const size_t nSampsRxBuff = rxStream->get_max_num_samps();
  int nRxChan = usrp->get_rx_num_channels();
  std::vector<std::vector<std::complex<float>>> rxBuffs(
      nRxChan, std::vector<std::complex<float>>(nSampsRxBuff));
  // Create a vector of pointers to each of the channel buffers
  std::vector<std::complex<float> *> rxBuffPtrs;
  for (size_t i = 0; i < nRxChan; i++)
    rxBuffPtrs.push_back(&rxBuffs[i].front());

  /*
   * Set up the threads
   */
  // Reset the USRP time and decide on a start time for Tx and Rx
  double startTime = 0.1;
  int nRxSamps = sampleRate * 3;
  usrp->set_time_now(0.0);
  boost::thread_group txThread, rxThread;
  txThread.create_thread(
      std::bind(&transmit, lfm, txBuffPtrs, txStream, startTime));
  rxThread.create_thread(
      std::bind(&receive, rxBuffPtrs, rxStream, startTime, nRxSamps));

  // Wait for threads to get back
  txThread.join_all();
  rxThread.join_all();

  std::cout << "Done!" << std::endl;
  return EXIT_SUCCESS;
}
