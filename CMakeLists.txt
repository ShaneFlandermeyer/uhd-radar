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


########################################################################
# Configure Compiler
########################################################################
set(CMAKE_CXX_STANDARD 17)
# Set up the build environment
# Use the shared UHD libs
option(UHD_USE_STATIC_LIBS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


########################################################################
# Find build dependencies
########################################################################
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(plasma-dsp REQUIRED)
find_package(UHD 4.0.0 REQUIRED)
find_package(Qt5 COMPONENTS Widgets)
# find_package(Qwt)
find_package(OpenMP)
if (${Qt5_FOUND})
  message(STATUS "Found Qt5")
endif()
# if (${Qwt_FOUND})
#   message(STATUS "Found Qwt")
# endif()
if (${OpenMP_FOUND})
  message(STATUS "Found OpenMP")
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
# Shared library case: All we need to do is link against UHD and Boost
message(STATUS "Linking against shared UHD library.")
link_libraries(${UHD_LIBRARIES} ${Boost_LIBRARIES})

########################################################################
# Add subdirectories
########################################################################
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(app)
