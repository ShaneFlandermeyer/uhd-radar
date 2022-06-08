list(APPEND
  UHD_RADAR_HEADERS
  receive.h
  transmit.h
  uhd_utils.h)
# # List of header files
# set(INCLUDE_HEADERS
# receive.h
# transmit.h
# uhd_utils.h)
# # Prepend the include directory path to each element in INCLUDE_HEADERS since
# # we'll be dealing with it globally
# list(TRANSFORM INCLUDE_HEADERS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
# # Make headers visible to the entire project
# set(INCLUDE_HEADERS "${INCLUDE_HEADERS}" PARENT_SCOPE)

# System-wide installation
install(FILES
  ${UHD_RADAR_HEADERS}
  DESTINATION ${UHD_RADAR_INCLUDE_DIR}
)