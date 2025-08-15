
# Specify cmake version - make sure we have the needed features
# cmake_minimum_required (VERSION 2.8)
cmake_minimum_required (VERSION 3.5)

# Define the project name and language
set (PROJECT_NAME vorpal)
project (${PROJECT_NAME} C CXX)

# Add a CMake module path for custom modules
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set (CMAKE_C_FLAGS_DEBUG "-O0 -g -Wall")
set (CMAKE_C_FLAGS_RELEASE "-O3 -Wall")

# Find OpenAL package
find_package(OpenAL REQUIRED)


# Where to look for when using #include <...>
include_directories (${OPENAL_INCLUDE_DIR})
include_directories (${CMAKE_SOURCE_DIR}/src)
include_directories (${CMAKE_BINARY_DIR}/externals/include)
include_directories(${CMAKE_BINARY_DIR}/src)

link_directories (${CMAKE_BINARY_DIR}/externals/lib)

set (PROJECT_EXPORT "${PROJECT_NAME}")

add_subdirectory (src)
add_subdirectory (externals)

# Install Vorpal library
install(TARGETS ${PROJECT_EXPORT}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install Vorpal headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/
    DESTINATION include/vorpal
    FILES_MATCHING PATTERN "*.h"
)

# Install ODA Panel patches
install (FILES
         ${CMAKE_SOURCE_DIR}/patches/vorpal_core.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_panel.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_plot.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_bus.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_parameter.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_seq16.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_metro.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_sample.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_env.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_filter.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_ogg.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_osc.pd
         ${CMAKE_SOURCE_DIR}/patches/vorpal_tri.pd
         DESTINATION patches
         CONFIGURATIONS Debug Release)
