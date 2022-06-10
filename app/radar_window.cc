#include "radar_window.h"

// #include <qwt/qwt_plot.h>

// #include <QFileDialog>

// #include "../ui/ui_radar_window.h"
// #include "receive.h"
// #include "transmit.h"

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
  connect(ui->stop_button, SIGNAL(clicked()), this,
          SLOT(on_stop_button_clicked()), Qt::UniqueConnection);

  // Qwt plotting setup
  ui->rx_plot->setTitle("Rx data");
  rx_data_curve = new QwtPlotCurve();
  rx_data_curve->setPen(QPen(Qt::red));
}

RadarWindow::~RadarWindow() { delete ui; }

void RadarWindow::read_calibration_json() {
  const std::string homedir = getenv("HOME");
  std::ifstream file(homedir + "/.uhd/delay_calibration.json");
  nlohmann::json json;
  delay_samps = 0;
  if (file) {
    file >> json;
    std::string radio_type = usrp->get_mboard_name();
    for (auto &config : json[radio_type]) {
      if (config["samp_rate"] == usrp->get_tx_rate() and
          config["master_clock_rate"] == usrp->get_master_clock_rate()) {
        delay_samps = config["delay"];
        break;
      }
    }
    if (delay_samps == 0)
      UHD_LOG_INFO("RadarWindow",
                   "Calibration file found, but no data exists for this "
                   "combination of radio, master clock rate, and sample rate");
  } else {
    UHD_LOG_INFO("RadarWindow", "No calibration file found");
  }

  file.close();
}

void RadarWindow::transmit(uhd::usrp::multi_usrp::sptr usrp,
                           std::vector<std::complex<float> *> buff_ptrs,
                           size_t num_pulses, size_t num_samps_pulse,
                           uhd::time_spec_t start_time) {
  bool first = true;
  uhd::set_thread_priority_safe(1);

  // static bool first = true;
  uhd::stream_args_t tx_stream_args("fc32", "sc16");
  uhd::tx_streamer::sptr tx_stream;
  // tx_stream.reset();
  tx_stream_args.channels.push_back(0);
  tx_stream = usrp->get_tx_stream(tx_stream_args);

  // Create metadata structure
  uhd::tx_metadata_t tx_md;
  tx_md.start_of_burst = true;
  tx_md.end_of_burst = false;
  tx_md.has_time_spec = (start_time.get_real_secs() > 0 ? true : false);
  tx_md.time_spec = start_time;
  while (not stop_button_clicked) {
    tx_stream->send(buff_ptrs, num_samps_pulse, tx_md, 0.5);
    tx_md.start_of_burst = false;
    tx_md.has_time_spec = false;
  }

  // Send mini EOB packet
  tx_md.has_time_spec = false;
  tx_md.end_of_burst = true;
  tx_stream->send("", 0, tx_md);
}

// void RadarWindow::write(std::complex<float> *ptr, size_t len) {
//   // uhd::set_thread_priority_safe(1);

//   file.write((const char *)ptr, len * sizeof(std::complex<float>));
//   return;
// }

void RadarWindow::receive(uhd::usrp::multi_usrp::sptr usrp,
                          std::vector<std::complex<float> *> buff_ptrs,
                          size_t num_samps, uhd::time_spec_t start_time) {
  uhd::set_thread_priority_safe(1);
  file = std::ofstream(ui->file_line_edit->text().toStdString(),
                       std::ios::out | std::ios::binary);
  size_t channels = buff_ptrs.size();
  std::vector<size_t> channel_vec;
  uhd::stream_args_t stream_args("fc32", "sc16");
  uhd::rx_streamer::sptr rx_stream;
  if (channel_vec.size() == 0) {
    for (size_t i = 0; i < channels; i++) {
      channel_vec.push_back(i);
    }
  }
  stream_args.channels = channel_vec;
  rx_stream = usrp->get_rx_stream(stream_args);

  uhd::rx_metadata_t md;

  // Set up streaming
  uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
  // stream_cmd.num_samps = num_samps;
  stream_cmd.time_spec = start_time;
  stream_cmd.stream_now = (start_time.get_real_secs() > 0 ? false : true);
  rx_stream->issue_stream_cmd(stream_cmd);

  size_t max_num_samps = rx_stream->get_max_num_samps();
  size_t num_samps_total = 0;
  std::vector<std::complex<float> *> buff_ptrs2(buff_ptrs.size());
  double timeout = 0.5 + start_time.get_real_secs();
  while (not stop_button_clicked) {
    // Move storing pointer to correct location
    for (size_t i = 0; i < channels; i++)
      buff_ptrs2[i] = &(buff_ptrs[i][num_samps_total]);

    // Sampling data
    size_t samps_to_recv = std::min(num_samps - num_samps_total, max_num_samps);

    size_t num_rx_samps =
        rx_stream->recv(buff_ptrs2, samps_to_recv, md, timeout);
    timeout = 0.5;

    num_samps_total += num_rx_samps;

    // handle the error code
    // if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) break;
    // if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) break;
    if (num_samps_total >= num_samps) {
      num_samps_total = 0;
      if (ui->file_write_checkbox->isChecked()) {
        // TODO: Can't write to a file quickly enough
        file.write((const char *)buff_ptrs[0], num_samps * sizeof(std::complex<float>));
      }

      // for (size_t i = 0; i < num_samps; i++) {
      //   write_queue.push(buff_ptrs[0][i]);
      // }
    }
  }
  // write_thread.join();
  stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
  stream_cmd.stream_now = true;
  rx_stream->issue_stream_cmd(stream_cmd);
  file.close();
}

void RadarWindow::pulse_doppler_worker() {
  static bool first = true;
  update_usrp();
  update_waveform();
  // boost::thread_group tx_thread;
  std::thread tx_thread, rx_thread;

  // Set up Tx buffer
  Eigen::ArrayXcf waveform_data = waveform.step().cast<std::complex<float>>();
  std::vector<std::complex<float>> tx_buff(
      waveform_data.data(), waveform_data.data() + waveform_data.size());
  std::vector<std::complex<float> *> tx_buff_ptrs;
  tx_buff_ptrs.push_back(&tx_buff.front());

  // Set up Rx buffer
  size_t num_samp_rx = waveform_data.size() * num_pulses_tx + delay_samps;
  std::vector<std::complex<float> *> rx_buff_ptrs;
  rx_buff = std::vector<std::complex<float>>(num_samp_rx, 0);
  rx_buff_ptrs.push_back(&rx_buff.front());

  size_t cpi_count = 0;

  uhd::time_spec_t start_time = usrp->get_time_now() + 0.2;
  tx_thread = std::thread(&RadarWindow::transmit, this, usrp, tx_buff_ptrs,
                          num_pulses_tx, waveform_data.size(), start_time);
  rx_thread = std::thread(&RadarWindow::receive, this, usrp, rx_buff_ptrs,
                          num_samp_rx, start_time);
  // file.close();
  tx_thread.join();
  rx_thread.join();
  // tx_thread.create_thread(boost::bind(&RadarWindow::transmit, usrp,
  //                                       tx_buff_ptrs, num_pulses_tx,
  //                                       waveform_data.size(), start_time));
  // tx_thread.join_all();
  // while (not stop_button_clicked) {
  //   start_time = usrp->get_time_now().get_real_secs() + 0.1;
  //   auto t1 = usrp->get_time_now();
  //   // Simultaneously transmit and receive the data
  //   tx_thread.create_thread(boost::bind(&RadarWindow::transmit, usrp,
  //                                       tx_buff_ptrs, num_pulses_tx,
  //                                       waveform_data.size(), start_time));
  // size_t n = uhd::radar::receive(usrp, rx_buff_ptrs, num_samp_rx,
  // start_time); tx_thread.join_all(); auto t2 = usrp->get_time_now();
  //   // std::cout << (t2 - t1).get_real_secs() << std::endl;
  //   if (ui->file_write_checkbox->isChecked()) {
  //     outfile.write(
  //         (char *)(rx_buff.data() + delay_samps),
  //         sizeof(std::complex<float>) * (rx_buff.size() + delay_samps));
  // }
  // rx_buff_ptrs.clear();
  // tx_buff_ptrs.clear();
  // rx_buff_ptrs.push_back(&rx_buff.front());
  // rx_buff_ptrs.push_back(&rx_buff_ch2->front());
  // tx_buff_ptrs.push_back(&tx_buff.front());

  // start_time += num_samp_rx/tx_rate + 0.2;

  //   cpi_count++;
  // }
  // outfile.close();
  std::cout << "Transmission successful" << std::endl << std::endl;
}

void RadarWindow::on_file_button_clicked() {
  QFileDialog dialog(this);
  dialog.setFileMode(QFileDialog::AnyFile);
  QString filename;
  if (dialog.exec()) {
    filename = dialog.selectedFiles().first();
    ui->file_line_edit->setText(filename);
  }
}

void RadarWindow::on_stop_button_clicked() {
  stop_button_clicked = true;
  pulse_doppler_thread.join();
}

void RadarWindow::on_start_button_clicked() {
  stop_button_clicked = false;
  pulse_doppler_thread = std::thread(&RadarWindow::pulse_doppler_worker, this);
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
    // Only need to reset the USRP sptr on the first run or whenever the sample
    // rate change requires a different master clock rate
    if (first or tx_rate != usrp->get_tx_rate() or
        rx_rate != usrp->get_rx_rate()) {
      first = false;
      usrp.reset();
      usrp = uhd::usrp::multi_usrp::make(tx_args);
    }
    usrp->set_tx_rate(tx_rate);
    usrp->set_rx_rate(rx_rate);
    usrp->set_tx_freq(tx_freq);
    usrp->set_rx_freq(rx_freq);
    usrp->set_tx_gain(tx_gain);
    usrp->set_rx_gain(rx_gain);
    // TODO: Add LO lock check for changing center frequency
    read_calibration_json();

    std::cout << "USRP successfully configured" << std::endl;
  } catch (const std::exception &e) {
    UHD_LOG_ERROR("RADAR_WINDOW", e.what());
  }
}

void RadarWindow::update_waveform() {
  double bandwidth = ui->bandwidth->text().toDouble();
  double pulse_width = ui->pulse_width->text().toDouble();
  double prf = ui->prf->text().toDouble();
  double samp_rate = ui->tx_rate->text().toDouble();

  waveform = plasma::LinearFMWaveform(bandwidth, pulse_width, prf, samp_rate);

  std::cout << "Waveform successfully configured" << std::endl;
}

void RadarWindow::on_waveform_update_button_clicked() { update_waveform(); }

void RadarWindow::on_usrp_update_button_clicked() { update_usrp(); }