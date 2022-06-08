#ifndef C3A1FFED_382D_4E7B_A366_CA7F8E902290
#define C3A1FFED_382D_4E7B_A366_CA7F8E902290

#include <uhd/usrp/multi_usrp.hpp>

namespace uhd {
namespace radar {

/**
 * @brief Receive samples from a USRP device
 *
 * @param usrp Device to use
 * @param buff_ptrs A vector where each element is a pointer to an array where
 * the data is stored for each channel.
 * @param num_samps Number of samples to receive
 * @param start_time Time to start receiving samples, relative to the USRP
 * device time. If this value is zero (or negative), the data is received
 * immediately. This does not account for the additional constant delay
 * introduced by the internal DSP on the USRP.
 * @return long int If the data was received successfully (i.e. no dropped
 * samples), this function returns the number of samples requested. If the
 * number of samples received is less than the number of samples requested, this
 * function returns the negative of the number of samples received.
 */
long int receive(uhd::usrp::multi_usrp::sptr usrp,
                 std::vector<std::complex<float> *> buff_ptrs, size_t num_samps,
                 uhd::time_spec_t start_time);

void handle_receive_errors(const uhd::rx_metadata_t &rx_meta);

}  // namespace radar
}  // namespace uhd

#endif /* C3A1FFED_382D_4E7B_A366_CA7F8E902290 */
