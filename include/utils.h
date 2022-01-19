#ifndef C7C305FC_6C7D_412C_A8E4_21228B58125B
#define C7C305FC_6C7D_412C_A8E4_21228B58125B
#include <uhd/usrp/multi_usrp.hpp>

// TODO: Choose a real namespace name
namespace uhd {
namespace radar {

/**
 * @brief Transmit a given number of pulses at a single PRF
 *
 * @param tx_stream UHD tx streamer object
 * @param tx_data A vector of vectors of complex numbers, where each "inner"
 * vector stores the waveform samples for a single channel
 * @param prf The pulse repetition frequency (Hz)
 * @param num_pulses_to_send Number of pulses to transmit
 * @param start_time Time to wait before the first transmission (s)
 */
void transmit(const uhd::tx_streamer::sptr &tx_stream,
              std::vector<std::vector<std::complex<double>>> &tx_data,
              double prf, size_t num_pulses_to_send, double start_time);

// TODO: Not yet implemented
void transmit(uhd::tx_streamer::sptr tx_stream,
              std::vector<std::vector<std::complex<double>>> tx_data,
              std::vector<double> prf_schedule, size_t num_pulses_to_send,
              double start_time);

void receive(const uhd::rx_streamer::sptr &rx_stream,
             std::vector<std::vector<std::complex<double>>> &rx_data,
             size_t num_samps_to_receive, double start_time);

}  // namespace radar
}  // namespace uhd

#endif /* C7C305FC_6C7D_412C_A8E4_21228B58125B */
