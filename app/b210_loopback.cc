#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>

#include "LinearFMWaveform.h"
#include "sigmf.h"
#include "sigmf_antenna_generated.h"
#include "sigmf_core_generated.h"

#define VERBOSE true
#define SAVE_DATA true

/*
 * Global variables
 */
sigmf::SigMF<sigmf::Global<core::DescrT, antenna::DescrT>,
             sigmf::Capture<core::DescrT>, sigmf::Annotation<core::DescrT>>
    txMeta;
sigmf::SigMF<sigmf::Global<core::DescrT, antenna::DescrT>,
             sigmf::Capture<core::DescrT>, sigmf::Annotation<core::DescrT>>
    rxMeta;

/*
 * Signal handlers
 */
static bool stopSignalCalled = false;
void sigIntHandler(int) { stopSignalCalled = true; }

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
              uhd::tx_streamer::sptr txStream, double startTime = 0.1,
              int nSampsToTransmit = 0, std::string filename = "") {
  bool writeToFile = true;
  // No filename specified. Don't write data to file
  if (filename.empty()) writeToFile = false;
  // Create an ofstream object for the data file
  // (use shared_ptr because ofstream is non-copyable)
  std::shared_ptr<std::ofstream> outfile(
      new std::ofstream(filename.c_str(), std::ofstream::binary));

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
  size_t nSampsTx;
  int nSampsTotal = 0;

  while (not stopSignalCalled and
         (nSampsTotal < nSampsToTransmit or nSampsToTransmit == 0)) {
    nSampsTx = txStream->send(buffs, nSampsPulse, txMeta, timeout);
    // Update metadata
    nSampsTotal += nSampsTx;
    sendTime += pri;
    txMeta.has_time_spec = true;
    txMeta.time_spec = uhd::time_spec_t(sendTime);
    txMeta.start_of_burst = false;
    if (writeToFile) {
      outfile->write((const char *)buffs[0],
                     nSampsTx * sizeof(std::complex<float>));
    }
  }
  // Send a mini EOB packet
  txMeta.end_of_burst = true;
  txStream->send("", 0, txMeta);
}

/*
 * Worker function to handle receive operation
 */
void receive(std::vector<std::complex<float> *> buffs,
             uhd::rx_streamer::sptr rxStream, double startTime = 0.1,
             int nSampsRequested = 0, std::string filename = "") {
  // Set up output file writing
  bool writeToFile = true;
  if (filename.empty()) writeToFile = false;
  // Create an ofstream object for the data file
  // (use shared_ptr because ofstream is non-copyable)
  std::shared_ptr<std::ofstream> outfile(
      new std::ofstream(filename.c_str(), std::ofstream::binary));
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
  // Timeout value for first call to recv()
  double timeout = 0.1 + startTime;
  int nSampsTotal = 0;
  // Main receive loop
  while (not stopSignalCalled and
         (nSampsTotal < nSampsRequested or nSampsRequested == 0)) {
    size_t nSampsRx = rxStream->recv(buffs, nSampsRxBuff, rxMeta, timeout);
    // After the initial start, we can make a smaller timeout
    startTime = 0.1;
    // Check for errors
    if (rxMeta.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
      std::cout << boost::format("Timeout while streaming") << std::endl;
      break;
    }
    if (rxMeta.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
      throw std::runtime_error(
          str(boost::format("Receiver error %s") % rxMeta.strerror()));
    }
    // TODO: Repeat this operation for every buffer/channel
    if (writeToFile) {
      outfile->write((const char *)buffs[0],
                     nSampsRx * sizeof(std::complex<float>));
    }
    // Update the sample count
    nSampsTotal += nSampsRx;
  }
  // Shut down the receiver
  streamCmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
  rxStream->issue_stream_cmd(streamCmd);

  // Close files
  outfile->close();
}

/**************************************************************************
 * Main function
 ***********************************************************************/
int UHD_SAFE_MAIN(int argc, char *argv[]) {
  std::signal(SIGINT, &sigIntHandler);
  // Filenames for metadata and IQ data
  std::string dataDir;
  std::string txDataFilename;
  std::string rxDataFilename;
  std::string txMetaFilename;
  std::string rxMetaFilename;
  // If we are actually saving data, set up the directory structure
  if (SAVE_DATA) {
    dataDir = "../data/";
    txDataFilename = "b210-loopback-tx.sigmf-data";
    rxDataFilename = "b210-loopback-rx.sigmf-data";
    txMetaFilename = "b210-loopback-tx.sigmf-meta";
    rxMetaFilename = "b210-loopback-rx.sigmf-meta";
    // If a data directory doesn't exist, make it
    boost::filesystem::path dir(dataDir);
    if (!(boost::filesystem::exists(dir))) {
      std::cout << "Data directory doesn't exist...creating" << std::endl;

      if (boost::filesystem::create_directories(dir))
        std::cout << "Successfully created data directory!" << std::endl;
    }
  }
    // USRP device setup
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

    // Check LO lock before continuing
    checkLoLock(usrp);

    // Waveform setup
    float bandwidth = 20e6;
    float pulsewidth = 10e-6;
    int nSampsPulse = std::round(sampleRate * pulsewidth);
    double prf = 10e3;
    double pri = 1 / prf;
    LinearFMWaveform lfm(bandwidth, pulsewidth, prf, sampleRate);
    auto waveform = lfm.generateWaveform();

    // Create stream objects
    uhd::stream_args_t streamArgs("fc32", "sc16");
    streamArgs.channels = txChanNums;
    uhd::tx_streamer::sptr txStream = usrp->get_tx_stream(streamArgs);
    uhd::rx_streamer::sptr rxStream = usrp->get_rx_stream(streamArgs);

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

    // Set up the threads

    // Reset the USRP time and decide on a start time for Tx and Rx
    double startTime = 1;
    int nSamps = sampleRate * 1;
    // Rx output is delayed by a constant (SDR-dependent) number of samples. To
    // get the correct rx length, sample an additional nRxOffsetSamps samples
    int nRxOffsetSamps = 165;
    usrp->set_time_now(0.0);
    boost::thread_group txThread, rxThread;
    txThread.create_thread(std::bind(&transmit, lfm, txBuffPtrs, txStream,
                                     startTime, nSamps,
                                     dataDir + txDataFilename));
    rxThread.create_thread(std::bind(&receive, rxBuffPtrs, rxStream, startTime,
                                     nSamps + nRxOffsetSamps,
                                     dataDir + rxDataFilename));

    // Wait for threads to get back
    txThread.join_all();
    rxThread.join_all();

    std::cout << boost::format("Successfully processed %d samples") % nSamps
              << std::endl;

    // Core global fields
    // TODO: Add the rest of the fields
    txMeta.global.access<core::GlobalT>().author =
        "Shane Flandermeyer shaneflandermeyer@gmail.com";
    rxMeta.global.access<core::GlobalT>().author =
        "Shane Flandermeyer shaneflandermeyer@gmail.com";
    txMeta.global.access<core::GlobalT>().description =
        "Basic loopback with an NI-2901 SDR";
    rxMeta.global.access<core::GlobalT>().description =
        "Basic loopback with an NI-2901 SDR";
    txMeta.global.access<core::GlobalT>().datatype = "cf32_le";
    rxMeta.global.access<core::GlobalT>().datatype = "cf32_le";
    // Antenna global fields
    txMeta.global.access<antenna::GlobalT>().gain = txGain;
    rxMeta.global.access<antenna::GlobalT>().gain = rxGain;
    // Write the metadata to a file
    // Convert the metadata to json
    nlohmann::json txMetaJson = json(txMeta);
    nlohmann::json rxMetaJson = json(rxMeta);

    // Write to file
    std::ofstream txMetaFile(dataDir + txMetaFilename);
    std::ofstream rxMetaFile(dataDir + rxMetaFilename);
    txMetaFile << std::setw(2) << txMetaJson << std::endl;
    rxMetaFile << std::setw(2) << rxMetaJson << std::endl;
    txMetaFile.close();
    rxMetaFile.close();

    return EXIT_SUCCESS;
  }
