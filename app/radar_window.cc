#include "radar_window.h"

#include "../ui/ui_radar_window.h"

void transmit(uhd::usrp::multi_usrp::sptr usrp,
              std::vector<std::complex<float> *> buff_ptrs, size_t num_pulses,
              size_t num_samps_pulse, uhd::time_spec_t start_time) {
  static bool first = true;

  static uhd::stream_args_t tx_stream_args("fc32", "sc16");
  if (first) {
    tx_stream_args.channels.push_back(0);
  }

  static uhd::tx_streamer::sptr tx_stream;
  if (first) {
    tx_stream = usrp->get_tx_stream(tx_stream_args);
  }

  // Create metadata structure
  static uhd::tx_metadata_t tx_md;
  tx_md.start_of_burst = true;
  tx_md.end_of_burst = false;
  tx_md.has_time_spec = true;
  tx_md.time_spec = start_time;

  for (size_t ipulse = 0; ipulse < num_pulses; ipulse++) {
    tx_stream->send(buff_ptrs, num_samps_pulse, tx_md, 0.5);
    tx_md.start_of_burst = false;
    tx_md.has_time_spec = false;
  }

  // Send mini EOB packet
  tx_md.has_time_spec = false;
  tx_md.end_of_burst = true;
  tx_stream->send("", 0, tx_md);

  first = false;
}

size_t receive(uhd::usrp::multi_usrp::sptr usrp,
               std::vector<std::complex<float> *> buff_ptrs, size_t len,
               uhd::time_spec_t start_time) {
  return 0;
}

RadarWindow::RadarWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::RadarWindow) {
  ui->setupUi(this);
  connect(ui->usrp_update_button, SIGNAL(clicked()), this,
          SLOT(on_usrp_update_button_clicked()), Qt::UniqueConnection);
  connect(ui->waveform_update_button, SIGNAL(clicked()), this,
          SLOT(on_waveform_update_button_clicked()), Qt::UniqueConnection);
  connect(ui->start_button, SIGNAL(clicked()), this,
          SLOT(on_start_button_clicked()), Qt::UniqueConnection);
}

RadarWindow::~RadarWindow() { delete ui; }

void RadarWindow::on_start_button_clicked() {
  Eigen::ArrayXcf waveform_data = waveform.step().cast<std::complex<float>>();
  std::vector<std::complex<float>> *tx_buff =
      new std::vector<std::complex<float>>(
          waveform_data.data(), waveform_data.data() + waveform_data.size());
  std::vector<std::complex<float> *> tx_buff_ptrs;
  tx_buff_ptrs.push_back(&tx_buff->front());
  usrp->set_time_now(0.0);
  transmit(usrp, tx_buff_ptrs, 2, waveform_data.size(),
           uhd::time_spec_t(2.0));
  std::cout << "Transmission successful" << std::endl;
}

void RadarWindow::on_waveform_update_button_clicked() {
  double bandwidth = ui->bandwidth->text().toDouble();
  double pulse_width = ui->pulse_width->text().toDouble();
  double prf = ui->prf->text().toDouble();
  double samp_rate = ui->samp_rate->text().toDouble();

  waveform = plasma::LinearFMWaveform(bandwidth, pulse_width, prf, samp_rate);

  std::cout << "Waveform successfully configured" << std::endl;
}

void RadarWindow::on_usrp_update_button_clicked() {
  // Tx parameters
  double tx_rate = ui->tx_rate->text().toDouble();
  double tx_freq = ui->tx_freq->text().toDouble();
  double tx_gain = ui->tx_gain->text().toDouble();
  std::string tx_args = ui->tx_args->text().toStdString();
  // Rx parameters
  double rx_rate = ui->rx_rate->text().toDouble();
  double rx_freq = ui->rx_freq->text().toDouble();
  double rx_gain = ui->rx_gain->text().toDouble();
  std::string rx_args = ui->rx_args->text().toStdString();
  if (strcmp(rx_args.c_str(), "")) {
    UHD_LOG_WARNING("RADAR_WINDOW", "Only Tx args are currently used");
  }

  // Configure the USRP device
  try {
    usrp = uhd::usrp::multi_usrp::make(tx_args);
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