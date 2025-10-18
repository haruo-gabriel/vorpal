
# Find the OpenAL package
# This sets the variables OPENAL_INCLUDE_DIR and OPENAL_LIBRARY
find_package (OpenAL REQUIRED)

# Define source code files
set (PROJECT_SRC
  vorpal.cxx
  engine.cxx
  audioserver.cxx
  dspserver.cxx
  parameter.cxx
  soundtrackevent.cxx
  pdinstance.cxx
  instancemanager.cxx
)

# Define source code headers
set (PROJECT_HEADERS
  vorpal.h
  engine.h
  status.h
  audioserver.h
  audiounit.h
  dspserver.h
  dspunit.h
  parameter.h
  portable.h
  soundtrackevent.h
  pdinstance.h
  instancemanager.h
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set (VORPAL_OSX 1)
endif ()

configure_file (
  portable.in.h
  portable.h
)

# Define our build target as a shared lib
add_library (${PROJECT_NAME} STATIC ${PROJECT_SRC} ${PROJECT_HEADERS})

target_include_directories(vorpal PRIVATE
  ${CMAKE_BINARY_DIR}/externals/include/libpd
  ${CMAKE_BINARY_DIR}/externals/include/libpd/util
)

# Link with OpenAL + libpd
target_link_libraries (
  ${PROJECT_NAME} PUBLIC
  pdcpp
  ${OPENAL_LIBRARY}
)

if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  message(STATUS "ENABLE_LIBPD_MULTI is ON: adding PDINSTANCE/PDTHREADS defs to vorpal target")
  target_compile_definitions(${PROJECT_NAME} PUBLIC PDINSTANCE=1 PDTHREADS=1)
endif()

# Set a few properties:
#  - Use C++11
#  - Output the lib file to <build directory>/lib
set_target_properties (
  ${PROJECT_NAME} PROPERTIES
  COMPILE_FLAGS "-Wall -pedantic -std=c++11 -fPIC"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Is there a better way to do this?
# Install the library target, with export option
install (TARGETS ${PROJECT_NAME}
         EXPORT ${PROJECT_EXPORT}
         DESTINATION lib
         CONFIGURATIONS Debug Release
         INCLUDES DESTINATION include)
# Install VORPAL's headers
install (FILES ${PROJECT_HEADERS}
         DESTINATION include/vorpal
         CONFIGURATIONS Debug Release)
# Export cmake target
install (EXPORT ${PROJECT_EXPORT}
         DESTINATION cmake)
