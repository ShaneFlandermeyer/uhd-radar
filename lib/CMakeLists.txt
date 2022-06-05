# Create the list of sources for the uhd-radar library
list(APPEND UHD_RADAR_SOURCES
receive.cc
transmit.cc
uhd_utils.cc)
# Prepend full path to the above sources
list(TRANSFORM UHD_RADAR_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
set(UHD_RADAR_SOURCES "${UHD_RADAR_SOURCES}" PARENT_SCOPE)
# Create the uhd-radar library
add_library(uhd_radar SHARED ${UHD_RADAR_SOURCES})

# Specify the library include dirs
target_include_directories(uhd_radar
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  PUBLIC $<INSTALL_INTERFACE:include>
)

# Link any necessary dependency libraries
target_link_libraries(uhd_radar PUBLIC plasma_dsp ${UHD_LIBRARIES} qwt-qt5 Qt5::Widgets)

target_include_directories(uhd_radar INTERFACE
    $<BUILD_INTERFACE:${JSON_INCLUDE_DIR}>
  )