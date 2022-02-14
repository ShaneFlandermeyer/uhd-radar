if (NOT UHD_USE_STATIC_LIBS)
  # Shared library case: All we need to do is link against UHD and Boost
  message(STATUS "Linking against shared UHD library.")
  link_libraries(${UHD_LIBRARIES} ${Boost_LIBRARIES})
else (NOT UHD_USE_STATIC_LIBS)
  # Static library case: We need to link to the static library AND manually link
  # to its dependencies
  message(STATUS "Linking against static UHD library.")
  link_libraries(
  ${UHD_STATIC_LIB_LINK_FLAG}
  ${UHD_STATIC_LIB_DEPS})
endif (NOT UHD_USE_STATIC_LIBS)
########################################################################
# Add executables
########################################################################
add_executable(init_usrp init_usrp.cc)
target_link_libraries(init_usrp plasma-dsp)

add_executable(plasma_loopback plasma_loopback.cc)
target_link_libraries(plasma_loopback plasma-dsp uhd-radar)

if (${Matplot++_FOUND})
  target_link_libraries(plasma_loopback Matplot++::matplot)
endif()

# # init_usrp is super basic and doesn't need any sources
# add_executable(init_usrp init_usrp.cc)
# # Add B210 loopback example and link it to our include directory
# add_executable(b210_loopback b210_loopback.cc  ${UHD_RADAR_SOURCES})
# target_link_libraries(b210_loopback uhd-radar)
# target_include_directories(b210_loopback PUBLIC ${CMAKE_SOURCE_DIR}/include)

