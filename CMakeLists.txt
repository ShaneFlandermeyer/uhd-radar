cmake_minimum_required(VERSION 3.20.1)
project(UHD_RADAR VERSION 1.0 LANGUAGES CXX)

########################################################################
# Configure Compiler
########################################################################
set(CMAKE_CXX_STANDARD 11)

# Set up the build environment
# Use the shared UHD libs
option(UHD_USE_STATIC_LIBS OFF)
# Add UHD as a dependency
find_package(UHD 4.0.0 REQUIRED)
# Add UHD boost requirements
set(UHD_BOOST_REQUIRED_COMPONENTS
  program_options
  system
  thread)
set(BOOST_MIN_VERSION 1.58)
# Include UHDBoost to do the actual finding
include(UHDBoost)

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${UHD_INCLUDE_DIRS}
)
link_directories(${Boost_LIBRARY_DIRS})

########################################################################
# Add subdirectories
########################################################################
add_subdirectory(src)

########################################################################
# Add subdirectories
########################################################################
# add_executable(init_usrp init_usrp.cc)