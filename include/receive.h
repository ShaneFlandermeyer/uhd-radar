#ifndef C3A1FFED_382D_4E7B_A366_CA7F8E902290
#define C3A1FFED_382D_4E7B_A366_CA7F8E902290

#include <uhd/usrp/multi_usrp.hpp>

namespace uhd {
namespace radar {

void receive(const uhd::rx_streamer::sptr &rx_stream,
             std::vector<std::vector<std::complex<double>>> &rx_data,
             size_t num_samps_to_receive, double start_time);

void handle_receive_errors(const uhd::rx_metadata_t &rx_meta);

}  // namespace radar
}  // namespace uhd

#endif /* C3A1FFED_382D_4E7B_A366_CA7F8E902290 */
