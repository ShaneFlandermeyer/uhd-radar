cmake_minimum_required(VERSION 3.16)
project(UHD_RADAR VERSION 1.0 LANGUAGES CXX)

########################################################################
# Configure Compiler
########################################################################
set(CMAKE_CXX_STANDARD 14)
# Set up the build environment
# Use the shared UHD libs
option(UHD_USE_STATIC_LIBS OFF)
# Add UHD as a dependency
find_package(UHD 4.0.0 REQUIRED)
# Add UHD boost requirements
set(UHD_BOOST_REQUIRED_COMPONENTS
  program_options
  filesystem
  system
  thread)
set(BOOST_MIN_VERSION 1.58)
# Include UHDBoost to do the actual finding
include(UHDBoost)
# Include miscellaneous directories that should be available to the whole project
include_directories( 
  ${Boost_INCLUDE_DIRS}
  ${UHD_INCLUDE_DIRS}
)
link_directories(${Boost_LIBRARY_DIRS})

########################################################################
# Set up libsigmf
########################################################################
# find_package(libsigmf REQUIRED)
message(STATUS "System-wide libsigmf not found. Building from submodule")
message(STATUS "TODO: Add an option for system-wide installation")
set(LIBSIGMF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/libsigmf/include
CACHE PATH "libsigmf include directory")
if (NOT IS_DIRECTORY ${LIBSIGMF_INCLUDE_DIR})
  message("sigmf submodule not found! Downloading...")
  execute_process(COMMAND git submodule update --init -- external/libsigmf
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT IS_DIRECTORY ${LIBSIGMF_INCLUDE_DIR})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/libsigmf
        ${CMAKE_CURRENT_BINARY_DIR}/libsigmf-build
        EXCLUDE_FROM_ALL)

########################################################################
# Add subdirectories
########################################################################
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(app)
