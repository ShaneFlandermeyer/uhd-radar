#include "radar_window.h"

#include <qwt/qwt_plot.h>

#include <QFileDialog>

#include "../ui/ui_radar_window.h"
#include "receive.h"
#include "transmit.h"

RadarWindow::RadarWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::RadarWindow) {
  ui->setupUi(this);
  // Signals and slots
  connect(ui->usrp_update_button, SIGNAL(clicked()), this,
          SLOT(on_usrp_update_button_clicked()), Qt::UniqueConnection);
  connect(ui->waveform_update_button, SIGNAL(clicked()), this,
          SLOT(on_waveform_update_button_clicked()), Qt::UniqueConnection);
  connect(ui->start_button, SIGNAL(clicked()), this,
          SLOT(on_start_button_clicked()), Qt::UniqueConnection);
  connect(ui->file_button, SIGNAL(clicked()), this,
          SLOT(on_file_button_clicked()), Qt::UniqueConnection);

  // Qwt plotting setup
  ui->rx_plot->setTitle("Rx data");
  rx_data_curve = new QwtPlotCurve();
  rx_data_curve->setPen(QPen(Qt::red));
}

RadarWindow::~RadarWindow() { delete ui; }

void RadarWindow::on_file_button_clicked() {
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::AnyFile);
  QString filename;
  if (dialog.exec()) {
    filename = dialog.selectedFiles().first();
    ui->file_line_edit->setText(filename);
  }
}

void RadarWindow::on_start_button_clicked() {
  static bool first = true;
  update_usrp();
  update_waveform();
  boost::thread_group tx_thread;

  // Set up Tx buffer
  Eigen::ArrayXcf waveform_data = waveform.step().cast<std::complex<float>>();
  std::vector<std::complex<float>> tx_buff(
          waveform_data.data(), waveform_data.data() + waveform_data.size());
  std::vector<std::complex<float> *> tx_buff_ptrs;
  tx_buff_ptrs.push_back(&tx_buff.front());

  // Set up Rx buffer
  size_t num_samp_rx = waveform_data.size() * num_pulses_tx;
  std::vector<std::complex<float> *> rx_buff_ptrs;
  std::vector<std::complex<float>> rx_buff(num_samp_rx, 0);
  rx_buff_ptrs.push_back(&rx_buff.front());
  // For unknown reasons, the first call to transmit() and receive() has a
  // different delay than on subsequent calls. The code below transmits and
  // receives a PRI of zeros to get a consistent delay for the rest of the
  // collection interval
  if (first) {
    first = false;
    std::vector<std::complex<float> *> temp_tx_buff_ptrs;
    std::vector<std::complex<float>> *temp_tx_buff =
        new std::vector<std::complex<float>>(waveform_data.size(), 0);
    temp_tx_buff_ptrs.push_back(&temp_tx_buff->front());
    tx_thread.create_thread(boost::bind(&uhd::radar::transmit, usrp,
                                        temp_tx_buff_ptrs, 1,
                                        waveform_data.size(), 0.0));
    size_t n =
        uhd::radar::receive(usrp, rx_buff_ptrs, waveform_data.size(), 0.0);
    tx_thread.join_all();
  }

  // Simultaneously transmit and receive the data
  uhd::time_spec_t time_now = usrp->get_time_now();
  tx_thread.create_thread(boost::bind(&uhd::radar::transmit, usrp, tx_buff_ptrs,
                                      num_pulses_tx, waveform_data.size(),
                                      time_now + tx_start_time));
  size_t n = uhd::radar::receive(usrp, rx_buff_ptrs, num_samp_rx,
                                 time_now + rx_start_time);
  tx_thread.join_all();

  // TODO: Make this an option in the gui
  if (ui->file_write_checkbox->isChecked()) {
    std::ofstream outfile(ui->file_line_edit->text().toStdString(),
                          std::ios::out | std::ios::binary);
    outfile.write((char *)rx_buff.data(),
                  sizeof(std::complex<float>) * rx_buff.size());
    outfile.close();
  }
  // Just plot the first pulse for now
  size_t nplot = num_samp_rx / num_pulses_tx;
  // size_t nplot = 300;
  std::vector<double> xdata(nplot);
  std::vector<double> ydata(nplot);
  for (int i = 0; i < nplot; i++) {
    xdata[i] = i;
    ydata[i] = abs(rx_buff.at(i));
  }
  rx_data_curve->setSamples(xdata.data(), ydata.data(), nplot);
  rx_data_curve->attach(ui->rx_plot);

  ui->rx_plot->replot();
  ui->rx_plot->show();

  std::cout << "Transmission successful" << std::endl << std::endl;
}

void RadarWindow::update_usrp() {
  // Tx parameters
  tx_rate = ui->tx_rate->text().toDouble();
  tx_freq = ui->tx_freq->text().toDouble();
  tx_gain = ui->tx_gain->text().toDouble();
  tx_start_time = uhd::time_spec_t(ui->tx_start_time->text().toDouble());
  tx_args = ui->tx_args->text().toStdString();
  // Rx parameters
  rx_rate = ui->rx_rate->text().toDouble();
  rx_freq = ui->rx_freq->text().toDouble();
  rx_gain = ui->rx_gain->text().toDouble();
  rx_start_time = uhd::time_spec_t(ui->rx_start_time->text().toDouble());
  rx_args = ui->rx_args->text().toStdString();
  num_pulses_tx = ui->num_pulses_tx->text().toDouble();
  if (strcmp(rx_args.c_str(), "")) {
    UHD_LOG_WARNING("RADAR_WINDOW", "Only Tx args are currently used");
  }

  // Configure the USRP device
  static bool first = true;
  try {
    if (first) {
      usrp = uhd::usrp::multi_usrp::make(tx_args);
      first = false;
    }
    usrp->set_tx_rate(tx_rate);
    usrp->set_rx_rate(rx_rate);
    usrp->set_tx_freq(tx_freq);
    usrp->set_rx_freq(rx_freq);
    usrp->set_tx_gain(tx_gain);
    usrp->set_rx_gain(rx_gain);
    // Sleep for 1 second to let the USRP settle
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "USRP successfully configured" << std::endl;
  } catch (const std::exception &e) {
    UHD_LOG_ERROR("RADAR_WINDOW", e.what());
  }
}

void RadarWindow::update_waveform() {
  double bandwidth = ui->bandwidth->text().toDouble();
  double pulse_width = ui->pulse_width->text().toDouble();
  double prf = ui->prf->text().toDouble();
  double samp_rate = ui->samp_rate->text().toDouble();

  waveform = plasma::LinearFMWaveform(bandwidth, pulse_width, prf, samp_rate);

  std::cout << "Waveform successfully configured" << std::endl;
}

void RadarWindow::on_waveform_update_button_clicked() { update_waveform(); }

void RadarWindow::on_usrp_update_button_clicked() { update_usrp(); }