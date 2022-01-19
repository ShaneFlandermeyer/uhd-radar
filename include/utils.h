#ifndef C7C305FC_6C7D_412C_A8E4_21228B58125B
#define C7C305FC_6C7D_412C_A8E4_21228B58125B
#include <uhd/usrp/multi_usrp.hpp>

namespace test {

void transmit(uhd::tx_streamer::sptr tx_stream,
              std::vector<std::vector<std::complex<double>>> tx_data,
              double prf, size_t num_pulses_to_send, double start_time);

// TODO: Not yet implemented
// void transmit(uhd::tx_streamer::sptr tx_stream,
//               std::vector<std::vector<std::complex<double>>> tx_data,
//               std::vector<double> prf_schedule, size_t num_pulses_to_send,
//               double start_time);

}  // namespace test

#endif /* C7C305FC_6C7D_412C_A8E4_21228B58125B */
