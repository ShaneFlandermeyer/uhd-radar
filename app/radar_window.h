#ifndef C8BFB65A_99BB_4C74_BE37_D5A14BD126D0
#define C8BFB65A_99BB_4C74_BE37_D5A14BD126D0

#include <plasma-dsp/linear_fm_waveform.h>

#include <QMainWindow>
#include <boost/thread.hpp>
#include <thread>
#include <uhd/usrp/multi_usrp.hpp>

namespace Ui {
class RadarWindow;
}

class RadarWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit RadarWindow(QWidget *parent = 0);
  ~RadarWindow() override;

  uhd::usrp::multi_usrp::sptr usrp;
  Ui::RadarWindow *ui;
  plasma::LinearFMWaveform waveform;

 private slots:
  void on_usrp_update_button_clicked();
  void on_waveform_update_button_clicked();
  void on_start_button_clicked();

 private:
  boost::thread_group pulse_doppler_thread;
};

// #include <qwt/qwt_thermo.h>
// #include <qwt/qwt_plot.h>
// #include <qwt/qwt_plot_curve.h>

// #include <QBoxLayout>
// #include <QPushButton>

// class Window : public QWidget {
//   // Must include Q_OBJECT macro for the Qt signals/slots framework to work
//   with
//   // this class
//   Q_OBJECT

//   public:
//     Window();

//     void timerEvent(QTimerEvent *);

//   private:
//     static constexpr int plotDataSize = 100;
//     static constexpr double gain = 7.5;

//     QPushButton *button;
//     QwtThermo *thermo;
//     QwtPlot *plot;
//     QwtPlotCurve *curve;

//     // Layout elements from Qt
//     QVBoxLayout *vLayout;
//     QHBoxLayout *hLayout;

//     // Data arrays for the plot
//     double xData[plotDataSize];
//     double yData[plotDataSize];

//     long count = 0;

//     void reset();
// };

#endif /* C8BFB65A_99BB_4C74_BE37_D5A14BD126D0 */
