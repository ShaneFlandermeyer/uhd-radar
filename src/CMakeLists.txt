add_executable(init_usrp init_usrp.cc)
if (NOT UHD_USE_STATIC_LIBS)
  # Shared library case: All we need to do is link against UHD and Boost
  message(STATUS "Linking against shared UHD library.")
  target_link_libraries(init_usrp ${UHD_LIBRARIES} ${Boost_LIBRARIES})
else (NOT UHD_USE_STATIC_LIBS)
  # Static library case: We need to link to the static library AND manually link
  # to its dependencies
  message(STATUS "Linking against static UHD library.")
  target_link_libraries(init_usrp
  ${UHD_STATIC_LIB_LINK_FLAG}
  ${UHD_STATIC_LIB_DEPS})
endif (NOT UHD_USE_STATIC_LIBS)