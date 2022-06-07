#ifndef C8BFB65A_99BB_4C74_BE37_D5A14BD126D0
#define C8BFB65A_99BB_4C74_BE37_D5A14BD126D0

#include <plasma_dsp/linear_fm_waveform.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>

#include <QFileDialog>
#include <QMainWindow>
#include <boost/thread.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>
#include <uhd/usrp/multi_usrp.hpp>

#include "../ui/ui_radar_window.h"
#include "receive.h"
#include "transmit.h"

namespace Ui {
class RadarWindow;
}

class RadarWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit RadarWindow(QWidget *parent = 0);
  ~RadarWindow() override;
  void update_usrp();
  void update_waveform();

  // Objects
  uhd::usrp::multi_usrp::sptr usrp;
  Ui::RadarWindow *ui;
  plasma::LinearFMWaveform waveform;

  // Parameters
  double tx_rate, rx_rate;
  double tx_freq, rx_freq;
  double tx_gain, rx_gain;
  uhd::time_spec_t tx_start_time, rx_start_time;
  std::string tx_args, rx_args;
  size_t num_pulses_tx;
  size_t delay_samps;

 private slots:
  void on_usrp_update_button_clicked();
  void on_waveform_update_button_clicked();
  void on_start_button_clicked();
  void on_file_button_clicked();

 private:
  boost::thread_group pulse_doppler_thread;
  QwtPlotCurve *rx_data_curve;

  void read_calibration_json();
};

#endif /* C8BFB65A_99BB_4C74_BE37_D5A14BD126D0 */
