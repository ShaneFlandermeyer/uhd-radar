#include <boost/program_options.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>

#include "LinearFMWaveform.h"
/***********************************************************************
 * Signal handlers
 **********************************************************************/
static bool stopSigCalled = false;
void sigIntHandler(int) { stopSigCalled = true; }

namespace po = boost::program_options;

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
  int chan = 0;
  
  uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(usrpArgs);
  // Set the sample rate
  usrp->set_tx_rate(sampleRate, chan);
  usrp->set_rx_rate(sampleRate, chan);
  // Set center frequency
  usrp->set_tx_freq(centerFreq, chan);
  usrp->set_rx_freq(centerFreq, chan);
  // Set RF gain
  usrp->set_tx_gain(txGain, chan);
  usrp->set_rx_gain(rxGain, chan);

  /*
   * TODO: Waveform setup
   */
  float bandwidth = 10e6;
  float pulsewidth = 10e-6;
  float prf = 1e3;
  LinearFMWaveform lfm(bandwidth, pulsewidth, prf, sampleRate);
  auto wav = lfm.generateWaveform();
  /*
   * Tx streamer setup
   */

  return EXIT_SUCCESS;
}
