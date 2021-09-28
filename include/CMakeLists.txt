# List of header files
set(INCLUDE_HEADERS Waveform.h LinearFMWaveform.h)
# Prepend the include directory path to each element in INCLUDE_HEADERS since
# we'll be dealing with it globally
list(TRANSFORM INCLUDE_HEADERS PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
# Make headers visible to the entire project
set(INCLUDE_HEADERS "${INCLUDE_HEADERS}" PARENT_SCOPE)