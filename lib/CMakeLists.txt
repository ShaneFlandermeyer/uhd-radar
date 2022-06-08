# Create the list of sources for the uhd_radar library
list(APPEND 
  UHD_RADAR_SOURCES
  receive.cc
  transmit.cc
  uhd_utils.cc)

# Prepend full path to the above sources
# list(TRANSFORM UHD_RADAR_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
set(UHD_RADAR_SOURCES "${UHD_RADAR_SOURCES}" PARENT_SCOPE)

# Create the uhd_radar library
add_library(uhd_radar SHARED ${UHD_RADAR_SOURCES})

# Specify the library include dirs
# target_include_directories(
#   uhd_radar PRIVATE 
#   PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/uhd_radar ${CMAKE_BINARY_DIR}/include/uhd_radar ${QWT_INCLUDE_DIR}>
#   PUBLIC $<INSTALL_INTERFACE:include>
# )

target_include_directories(
  uhd_radar PRIVATE
  PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/uhd_radar>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include/uhd_radar>
  PUBLIC $<BUILD_INTERFACE:${QWT_INCLUDE_DIR}>
  PUBLIC $<INSTALL_INTERFACE:include>)

# Link any necessary dependency libraries
target_link_libraries(uhd_radar PUBLIC 
  plasma_dsp 
  ${UHD_LIBRARIES} 
  ${QWT_LIBRARY} 
  Qt5::Widgets
  nlohmann_json::nlohmann_json
)

# Install the shared library file
install(TARGETS uhd_radar
  EXPORT uhd_radar
  DESTINATION ${UHD_RADAR_LIB_DIR})
# Install the cmake config file
install(EXPORT uhd_radar
  DESTINATION ${UHD_RADAR_CMAKE_DIR})