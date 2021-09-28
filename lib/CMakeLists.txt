# Create the list of sources for the uhd-radar library
list(APPEND UHD_RADAR_SOURCES
  Waveform.cc
  LinearFMWaveform.cc)
# Prepend full path to the above sources
list(TRANSFORM UHD_RADAR_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
set(UHD_RADAR_SOURCES "${UHD_RADAR_SOURCES}" PARENT_SCOPE)
# Create the uhd-radar library
add_library(uhd-radar SHARED ${UHD_RADAR_SOURCES})
# Link any necessary dependency libraries
target_link_libraries(uhd-radar)
# Specify the library include dirs
target_include_directories(uhd-radar
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>
)
