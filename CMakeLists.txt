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
find_package(libsigmf REQUIRED)
# Add UHD boost requirements
set(UHD_BOOST_REQUIRED_COMPONENTS
  program_options
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
# Add subdirectories
########################################################################
message(STATUS "Building Flatbuffers")
# Initialize Flatbuffers submodule if unititialized
set(FLATBUFFERS_INCLUDE_DIR 
  ${CMAKE_CURRENT_SOURCE_DIR}/external/flatbuffers/include
  CACHE PATH "flatbuffers include directory")
if (NOT IS_DIRECTORY ${FLATBUFFERS_INCLUDE_DIR})
    message("Flatbuffers submodule not found! Downloading...")
    execute_process(COMMAND git submodule update --init -- external/flatbuffers
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif(NOT IS_DIRECTORY ${FLATBUFFERS_INCLUDE_DIR})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/flatbuffers
        ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers-build
        EXCLUDE_FROM_ALL)

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(app)
