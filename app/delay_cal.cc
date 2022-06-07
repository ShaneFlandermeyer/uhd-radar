#include <plasma_dsp/linear_fm_waveform.h>
#include <plasma_dsp/processing.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <stdlib.h>

#include <QApplication>
#include <boost/thread.hpp>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>

#include "receive.h"
#include "transmit.h"

int main(int argc, char *argv[]) {
  uhd::set_thread_priority_safe();

  // Initialzie the USRP object
  double freq = 5e9;
  double start_time = 0.2;
  // double tx_gain = 50;
  // double rx_gain = 40;
  double tx_gain = 10;
  double rx_gain = 10;
  std::string args = "";
  boost::thread_group tx_thread;

  // std::vector<double> samp_rates = {10e6, 20e6, 30e6, 40e6, 50e6};
  std::vector<double> samp_rates = {12.5e6, 25e6, 50e6, 100e6, 200e6};
  // std::vector<double> samp_rates = {10e6};
  std::vector<double> master_clock_rates(samp_rates.size());
  std::vector<size_t> delays(samp_rates.size());
  uhd::usrp::multi_usrp::sptr usrp;
  std::string radio_type;
  for (int i = 0; i < samp_rates.size(); i++) {
    // Set up the USRP device
    usrp = uhd::usrp::multi_usrp::make(args);
    usrp->set_tx_freq(freq);
    usrp->set_rx_freq(freq);
    usrp->set_tx_gain(tx_gain);
    usrp->set_rx_gain(rx_gain);
    usrp->set_tx_rate(samp_rates[i]);
    usrp->set_rx_rate(samp_rates[i]);

    master_clock_rates[i] = usrp->get_master_clock_rate();
    if (i == 0) {
      radio_type = usrp->get_mboard_name();
    }
    // Initialize the waveform object
    double bandwidth = samp_rates[i] / 2;
    double pulse_width = 20e-6;
    double prf = 10e3;
    plasma::LinearFMWaveform waveform(bandwidth, pulse_width, prf,
                                      samp_rates[i]);
    Eigen::ArrayXcf waveform_data = waveform.step().cast<std::complex<float>>();

    // Set up Tx buffer
    std::vector<std::complex<float>> *tx_buff =
        new std::vector<std::complex<float>>(
            waveform_data.data(), waveform_data.data() + waveform_data.size());
    std::vector<std::complex<float> *> tx_buff_ptrs;
    tx_buff_ptrs.push_back(&tx_buff->front());

    // Set up Rx buffer
    size_t num_samp_rx = waveform_data.size();
    std::vector<std::complex<float> *> rx_buff_ptrs;
    std::vector<std::complex<float>> rx_buff(num_samp_rx, 0);
    rx_buff_ptrs.push_back(&rx_buff.front());

    // Send the  data to be used for calibration
    uhd::time_spec_t time_now = usrp->get_time_now();
    tx_thread.create_thread(boost::bind(&uhd::radar::transmit, usrp,
                                        tx_buff_ptrs, 1, waveform_data.size(),
                                        time_now + start_time));
    long int n = uhd::radar::receive(usrp, rx_buff_ptrs, num_samp_rx,
                                     time_now + start_time);
    tx_thread.join_all();

    // Create the matched filter
    Eigen::ArrayXcf x = Eigen::Map<Eigen::ArrayXcf, Eigen::Unaligned>(
        rx_buff.data(), rx_buff.size());
    Eigen::ArrayXcf h =
        Eigen::Map<Eigen::ArrayXcf, Eigen::Unaligned>(
            waveform_data.data(), round(samp_rates[i] * pulse_width))
            .conjugate();
    h.reverseInPlace();

    // Do convolution and compute the delay of the matched filter peak
    Eigen::ArrayXcf y = plasma::conv(x, h);
    size_t argmax = 0;
    for (size_t i = 0; i < y.size(); i++) {
      argmax = (abs(y[i]) > abs(y[argmax])) ? i : argmax;
    }
    delays[i] = argmax - h.size();

    // Reset the device for the next configuration
    usrp.reset();
  }

  // TODO: Make the filename a parameter
  // Save the calibration results to a json file
  const std::string homedir = getenv("HOME");
  // Create uhd hidden folder if it doesn't already exist
  if (not std::filesystem::exists(homedir + "/.uhd")) {
    std::filesystem::create_directory(homedir + "/.uhd");
  }
  // If data already exists in the file, keep it
  nlohmann::json json;
  std::ifstream prev_data(homedir + "/.uhd/delay_calibration.json");
  if (prev_data) prev_data >> json;
  prev_data.close();
  // Write the data to delay_calibration.json
  std::ofstream outfile(homedir + "/.uhd/delay_calibration.json");
  for (int i = 0; i < samp_rates.size(); i++) {
    // Check if radio/clock rate/sample rate triplet already exists
    bool exists = false;
    for (auto &config : json[radio_type]) {
      if (config["samp_rate"] == samp_rates[i] and
          config["master_clock_rate"] == master_clock_rates[i]) {
        config["delay"] = delays[i];
        exists = true;
        break;
      }
    }
    // Calibration configuration not found, add it
    if (not exists)
      json[radio_type].push_back({{"master_clock_rate", master_clock_rates[i]},
                                  {"samp_rate", samp_rates[i]},
                                  {"delay", delays[i]}});
  }
  outfile << json.dump(true);
  outfile.close();

  std::cout << json[radio_type][0] << std::endl;

  return EXIT_SUCCESS;
}