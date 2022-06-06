#include <plasma_dsp/linear_fm_waveform.h>
#include <plasma_dsp/signal_processing.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>

#include <QApplication>
#include <boost/thread.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/thread.hpp>

#include "receive.h"
#include "transmit.h"

class Window : public QWidget {
 public:
  Window() {
    plot = new QwtPlot();
    curve = new QwtPlotCurve();
    curve->setPen(QPen(Qt::blue));
  }
  QwtPlot *plot;
  QwtPlotCurve *curve;
};

/**
 * @brief Naive implementation of 1D convolution
 *
 * @param x
 * @param y
 * @return Eigen::ArrayXcf
 */
Eigen::ArrayXcf conv(const Eigen::ArrayXcf &x, const Eigen::ArrayXcf &y) {
  const int nx = x.size();
  const int ny = y.size();
  const int n = nx + ny - 1;
  Eigen::ArrayXcf out = Eigen::ArrayXcf::Zero(n);
  for (int i = 0; i < n; ++i) {
    const int jmn = (i >= ny - 1) ? i - (ny - 1) : 0;
    const int jmx = (i < nx - 1) ? i : nx - 1;
    for (auto j(jmn); j <= jmx; ++j) {
      out[i] += (x[j] * y[i - j]);
    }
  }
  return out;
}

int main(int argc, char *argv[]) {
  uhd::set_thread_priority_safe();
  boost::thread_group tx_thread;

  // Initialzie the USRP object
  double rate = 10e6;
  double freq = 5e9;
  double start_time = 0.5;
  double tx_gain = 50;
  double rx_gain = 40;
  std::string args = "";
  uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);
  usrp->set_tx_rate(rate);
  usrp->set_rx_rate(rate);
  usrp->set_tx_freq(freq);
  usrp->set_rx_freq(freq);
  usrp->set_tx_gain(tx_gain);
  usrp->set_rx_gain(rx_gain);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Initialize the waveform object
  double bandwidth = 1e6;
  double pulse_width = 100e-6;
  double prf = 1e3;
  plasma::LinearFMWaveform waveform(bandwidth, pulse_width, prf, rate);

  // Set up Tx buffer
  Eigen::ArrayXcf waveform_data = waveform.step().cast<std::complex<float>>();
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

  // Send zeros to get a consistent delay
  std::vector<std::complex<float> *> temp_tx_buff_ptrs;
  std::vector<std::complex<float>> *temp_tx_buff =
      new std::vector<std::complex<float>>(waveform_data.size(), 0);
  temp_tx_buff_ptrs.push_back(&temp_tx_buff->front());
  tx_thread.create_thread(boost::bind(&uhd::radar::transmit, usrp,
                                      temp_tx_buff_ptrs, 1,
                                      waveform_data.size(), 0.0));
  uhd::radar::receive(usrp, rx_buff_ptrs, waveform_data.size(), 0.0);
  tx_thread.join_all();
  // Send the actual data to be used for calibration
  uhd::time_spec_t time_now = usrp->get_time_now();
  tx_thread.create_thread(boost::bind(&uhd::radar::transmit, usrp, tx_buff_ptrs,
                                      1, waveform_data.size(),
                                      time_now + start_time));
  long int n = uhd::radar::receive(usrp, rx_buff_ptrs, num_samp_rx,
                      time_now + start_time + 82 / rate);
  tx_thread.join_all();

  // Create the matched filter
  Eigen::ArrayXcf x = Eigen::Map<Eigen::ArrayXcf, Eigen::Unaligned>(
      rx_buff.data(), rx_buff.size());
  Eigen::ArrayXcf h = Eigen::Map<Eigen::ArrayXcf, Eigen::Unaligned>(
      waveform_data.data(), round(rate * pulse_width));
  h = h.conjugate();
  h.reverseInPlace();
  Eigen::ArrayXcf y = conv(x, h);

  size_t argmax = 0;
  for (size_t i = 0; i < y.size(); i++) {
    argmax = (abs(y[i]) > abs(y[argmax])) ? i : argmax;
  }

  size_t delay = argmax - h.size();

  // Plot the pulse
  QApplication app(argc, argv);
  Window window;
  double xdata[n];
  double ydata[n];
  for (int i = 0; i < n; i++) {
    xdata[i] = i;
    ydata[i] = abs(rx_buff.at(i));
  }
  window.curve->setSamples(xdata, ydata, n);
  window.curve->attach(window.plot);
  // window.plot->setAxisScale(QwtPlot::yLeft, ymax - 70, ymax + 5);
  window.plot->replot();
  window.plot->show();
  app.exec();

  return EXIT_SUCCESS;
}