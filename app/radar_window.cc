#include "radar_window.h"

#include "../ui/ui_radar_window.h"

void transmit(uhd::usrp::multi_usrp::sptr usrp,
              std::vector<std::complex<float> *> buff_ptrs, size_t num_pulses,
              size_t num_samps_pulse, uhd::time_spec_t start_time) {
  static bool first = true;

  static uhd::stream_args_t tx_stream_args("fc32", "sc16");
  static uhd::tx_streamer::sptr tx_stream;
  if (first) {
    tx_stream_args.channels.push_back(0);
    tx_stream = usrp->get_tx_stream(tx_stream_args);
    first = false;
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
}

size_t receive(uhd::usrp::multi_usrp::sptr usrp,
               std::vector<std::complex<float> *> buff_ptrs, size_t num_samps,
               uhd::time_spec_t start_time) {
  static bool first = true;

  // static size_t channels = usrp->get_rx_num_channels();
  static size_t channels = 1;
  static std::vector<size_t> channel_vec;
  static uhd::stream_args_t stream_args("fc32", "sc16");
  static uhd::rx_streamer::sptr rx_stream;

  if (first) {
    if (channel_vec.size() == 0) {
      for (size_t i = 0; i < channels; i++) {
        channel_vec.push_back(i);
      }
    }
    stream_args.channels = channel_vec;
    rx_stream = usrp->get_rx_stream(stream_args);
    first = false;
  }

  uhd::rx_metadata_t md;

  // Set up streaming
  uhd::stream_cmd_t stream_cmd(
      uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
  stream_cmd.num_samps = num_samps;
  stream_cmd.stream_now = false;
  stream_cmd.time_spec = start_time;
  rx_stream->issue_stream_cmd(stream_cmd);

  size_t max_num_samps = rx_stream->get_max_num_samps();
  size_t num_samps_total = 0;
  std::vector<std::complex<float> *> buff_ptrs2(buff_ptrs.size());
  while(num_samps_total < num_samps) {
    // Move storing pointer to correct location
    for (size_t i = 0; i < channels; i++)
      buff_ptrs2[i] = &(buff_ptrs[i][num_samps_total]);

    // Sampling data
    size_t samps_to_recv = std::min(num_samps - num_samps_total, max_num_samps);
    size_t num_rx_samps = rx_stream->recv(buff_ptrs2, samps_to_recv, md, 0.5);

    num_samps_total += num_rx_samps;

    // handle the error code
    if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) break;
    if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) break;
  }

  first = false;
  if (num_samps_total < num_samps)
    return -((long int)num_samps_total);
  else
    return (long int)num_samps_total;
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
  // Set up Tx buffer
  boost::thread_group tx_thread;
  Eigen::ArrayXcf waveform_data = waveform.step().cast<std::complex<float>>();
  std::vector<std::complex<float>> *tx_buff =
      new std::vector<std::complex<float>>(
          waveform_data.data(), waveform_data.data() + waveform_data.size());
  std::vector<std::complex<float> *> tx_buff_ptrs;
  tx_buff_ptrs.push_back(&tx_buff->front());

  // Set up Rx buffer
  size_t num_samp_rx = waveform_data.size()*num_pulses_tx;
  std::vector<std::complex<float>*> rx_buff_ptrs;
  std::vector<std::complex<float>>* rx_buff = new std::vector<std::complex<float>>(num_samp_rx,0);
  rx_buff_ptrs.push_back(&rx_buff->front());
  
  usrp->set_time_now(0.0);
  tx_thread.create_thread(boost::bind(&transmit, usrp, tx_buff_ptrs,num_pulses_tx,waveform_data.size(),tx_start_time));
  size_t n = receive(usrp,rx_buff_ptrs, num_samp_rx,rx_start_time);

  tx_thread.join_all();
  

  // TODO: Make this an option
  std::ofstream outfile("/home/shane/gui_output.dat", std::ios::out | std::ios::binary);
  outfile.write((char*)rx_buff->data(), sizeof(std::complex<float>) * rx_buff->size());
  outfile.close();

  std::cout << "Transmission successful" << std::endl;

  delete rx_buff;
  delete tx_buff;
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