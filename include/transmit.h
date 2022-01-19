#ifndef FCA0C475_B76B_49D6_A57C_F204E70DDF27
#define FCA0C475_B76B_49D6_A57C_F204E70DDF27

#include <uhd/usrp/multi_usrp.hpp>

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

}  // namespace radar
}  // namespace uhd

#endif /* FCA0C475_B76B_49D6_A57C_F204E70DDF27 */
