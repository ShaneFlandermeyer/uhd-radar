#ifndef F8FB566B_9396_4CC2_93B0_F6ED316E0991
#define F8FB566B_9396_4CC2_93B0_F6ED316E0991

#include <uhd/usrp/multi_usrp.hpp>
namespace uhd {
namespace radar {

/**
 * @brief Check for LO lock in the transmitter and receiver
 * 
 * TODO: This currently only checks channel 0 (i.e., assumes monostatic)
 *
 * @param usrp Shared pointer (sptr) to the USRP object
 */

void check_lo_lock(uhd::usrp::multi_usrp::sptr usrp);
}  // namespace radar
}  // namespace uhd

#endif /* F8FB566B_9396_4CC2_93B0_F6ED316E0991 */
