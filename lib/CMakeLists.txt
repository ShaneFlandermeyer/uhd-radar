list(APPEND uhd_radar_sources
  Waveform.cc)

set(uhd_radar_sources "${uhd_radar_sources}" PARENT_SCOPE)
add_library(uhd-radar SHARED ${uhd_radar_sources})
target_link_libraries(uhd-radar)
target_include_directories(uhd-radar
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>
)
