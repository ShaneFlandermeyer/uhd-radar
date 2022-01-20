#include "uhd_utils.h"

namespace uhd {
namespace radar {

void check_lo_lock(uhd::usrp::multi_usrp::sptr usrp) {
  std::vector<std::string> tx_sensor_names = usrp->get_tx_sensor_names(0);
  std::vector<std::string> rx_sensor_names = usrp->get_rx_sensor_names(0);
  // Check if tx lo is locked
  if (std::find(tx_sensor_names.begin(), tx_sensor_names.end(), "lo_locked") !=
      tx_sensor_names.end()) {
    uhd::sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked", 0);
    std::cout << boost::format("Checking Tx (channel %d) %s") % 0 %
                     lo_locked.to_pp_string()
              << std::endl;
    UHD_ASSERT_THROW(lo_locked.to_bool());
  }

  // Check if rx lo is locked
  if (std::find(rx_sensor_names.begin(), rx_sensor_names.end(), "lo_locked") !=
      rx_sensor_names.end()) {
    uhd::sensor_value_t lo_locked = usrp->get_rx_sensor("lo_locked", 0);
    std::cout << boost::format("Checking Rx (channel %d) %s") % 0 %
                     lo_locked.to_pp_string()
              << std::endl;
    UHD_ASSERT_THROW(lo_locked.to_bool());
  }
}
}  // namespace radar

}  // namespace uhd