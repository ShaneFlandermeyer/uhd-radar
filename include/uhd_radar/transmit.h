#ifndef FCA0C475_B76B_49D6_A57C_F204E70DDF27
#define FCA0C475_B76B_49D6_A57C_F204E70DDF27

#include <uhd/usrp/multi_usrp.hpp>

namespace uhd {
namespace radar {

/**
 * @brief Transmit pulsed data using a USRP device
 *
 * @param usrp The device to be used
 * @param buff_ptrs Data to be transmitted through each channel
 * @param num_pulses Number of pulses (iterations through buff_ptrs) to transmit
 * @param num_samps_pulse Length of each data vector in buff_ptrs
 * @param start_time Time to send the first pulse in the sequence, relative to
 * the USRP time source. If this value is zero (or negative), the data is sent
 * immediately.
 */
void transmit(uhd::usrp::multi_usrp::sptr usrp,
              std::vector<std::complex<float> *> buff_ptrs, size_t num_pulses,
              size_t num_samps_pulse, uhd::time_spec_t start_time);

}  // namespace radar
}  // namespace uhd

#endif /* FCA0C475_B76B_49D6_A57C_F204E70DDF27 */
