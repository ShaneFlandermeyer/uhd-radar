cmake_minimum_required(VERSION 3.16)
project(uhd-radar VERSION 0.1 LANGUAGES CXX)

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
  message(
    FATAL_ERROR
      "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles."
  )
endif()

# ##############################################################################
# Build options
# ##############################################################################
option(ENABLE_OPENMP "ENABLE_OPENMP" ON)
option(ENABLE_MATPLOT++ "ENABLE_MATPLOT++" ON)

########################################################################
# Configure Compiler
########################################################################
set(CMAKE_CXX_STANDARD 20)
# Set up the build environment
# Use the shared UHD libs
option(UHD_USE_STATIC_LIBS OFF)

########################################################################
# Find build dependencies
########################################################################
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(plasma-dsp REQUIRED)
find_package(UHD 4.0.0 REQUIRED)

# TODO: I don't think I should have to do this, since plasma-dsp already links
# to these in its own project
target_link_libraries(plasma-dsp INTERFACE Eigen3::Eigen fftw3)

if(${ENABLE_OPENMP})
  find_package(OpenMP REQUIRED)
  target_link_libraries(plasma-dsp INTERFACE OpenMP::OpenMP_CXX)
endif()

if(${ENABLE_MATPLOT++})
  find_package(Matplot++ REQUIRED)
  set(Matplot++_FOUND TRUE)
endif()


########################################################################
# Set up UHD/Boost
########################################################################
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
