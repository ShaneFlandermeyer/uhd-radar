cmake_minimum_required(VERSION 3.16)
project(UHD_RADAR VERSION 1.0 LANGUAGES CXX)

# TODO: Remove these options
option(USE_SYSTEM_LIBSIGMF "Attempt to use the system installed libsigmf library" OFF)
option(USE_SYSTEM_JSON "Attempt to use the system installed nlohmann JSON library" OFF)
option(USE_SYSTEM_FLATBUFFERS "Attempt to use the system installed flatbuffers library and flatc" OFF)


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

# ########################################################################
# # Set up nlohmann json
# ########################################################################
# message(STATUS "System-wide nlohmann JSON not found. Building from submodule")
# message(STATUS "TODO: Add an option for system-wide installation")
# set(JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/json/include
# CACHE PATH "json include directory")
# if (NOT IS_DIRECTORY ${JSON_INCLUDE_DIR})
#   message("JSON submodule not found! Downloading...")
#   execute_process(COMMAND git submodule update --init -- external/json
#   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
# endif(NOT IS_DIRECTORY ${JSON_INCLUDE_DIR})
# 
# 
# ########################################################################
# # Set up flatbuffers
# ########################################################################
# message(STATUS "System-wide flatbuffers not found. Building from submodule")
# message(STATUS "TODO: Add an option for system-wide installation")
# set(FLATBUFFERS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/flatbuffers/include")
# # Initialize Flatbuffers submodule if unititialized
# set(FLATBUFFERS_INCLUDE_DIR 
#   ${CMAKE_CURRENT_SOURCE_DIR}/external/flatbuffers/include
#   CACHE PATH "flatbuffers include directory")

# if (NOT IS_DIRECTORY ${FLATBUFFERS_INCLUDE_DIR})
#     message("Flatbuffers submodule not found! Downloading...")
#     execute_process(COMMAND git submodule update --init -- external/flatbuffers
#       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
# endif(NOT IS_DIRECTORY ${FLATBUFFERS_INCLUDE_DIR})
# # Silence the warning from the flatbuffers build process
# set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/flatbuffers
#         ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers-build
#         EXCLUDE_FROM_ALL)

# # ########################################################################
# # # Set up nlohmann json
# # ########################################################################
# set(JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/json/include
# CACHE PATH "json include directory")
# if (NOT IS_DIRECTORY ${JSON_INCLUDE_DIR})
#   message("JSON submodule not found! Downloading...")
#   execute_process(COMMAND git submodule update --init -- external/json
#   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
# endif(NOT IS_DIRECTORY ${JSON_INCLUDE_DIR})


########################################################################
# Add subdirectories
########################################################################
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(app)
