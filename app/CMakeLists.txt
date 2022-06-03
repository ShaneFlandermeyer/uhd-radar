
########################################################################
# Add executables
########################################################################
add_executable(init_usrp init_usrp.cc)
target_link_libraries(init_usrp PUBLIC uhd_radar)

# add_executable(plasma-loopback plasma-loopback.cc)
# target_link_libraries(plasma-loopback plasma-dsp uhd-radar)


# # init_usrp is super basic and doesn't need any sources
# add_executable(init_usrp init_usrp.cc)
# # Add B210 loopback example and link it to our include directory
# add_executable(b210_loopback b210_loopback.cc  ${UHD_RADAR_SOURCES})
# target_link_libraries(b210_loopback uhd-radar)
# target_include_directories(b210_loopback PUBLIC ${CMAKE_SOURCE_DIR}/include)

