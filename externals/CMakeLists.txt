
# Build libpd as a dependency of liboda
#add_custom_target (
#  libpd
#  COMMAND git submodule update --init
#  COMMAND make libpd
#  COMMAND make cpplib UTIL=true EXTRA=true
#  COMMAND cmake -E make_directory ${CMAKE_BINARY_DIR}/externals/lib
#  COMMAND make install prefix=${CMAKE_BINARY_DIR}/externals
#  COMMAND cmake -E copy libs/libpdcpp.so ${CMAKE_BINARY_DIR}/externals/lib
#  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/externals/libpd
#  COMMENT "Building libpd"
#)

###############################################################################

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/endian_test.c ${CMAKE_BINARY_DIR}/endian_test.c COPYONLY)

# Define source code files
set (PD_SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_arithmetic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_array.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_ctl.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_dac.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_delay.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_fft.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_fft_fftsg.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_filter.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_global.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_math.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_misc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_osc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_resample.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_soundfile.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_soundfile_next.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_soundfile_caf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_soundfile_aiff.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_soundfile_wave.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/d_ugen.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_all_guis.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_array.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_bang.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_canvas.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_clone.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_editor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_editor_extras.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_graph.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_guiconnect.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_io.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_mycanvas.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_numbox.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_radio.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_readwrite.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_rtext.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_scalar.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_slider.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_template.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_text.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_toggle.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_traversal.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_undo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/g_vumeter.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/m_atom.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/m_binbuf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/m_class.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/m_conf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/m_glob.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/m_memory.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/m_obj.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/m_pd.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/m_sched.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_audio.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_audio_dummy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_file.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_inter.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_inter_gui.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_loader.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_net.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_path.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_print.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/s_utf8.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_acoustics.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_arithmetic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_array.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_connective.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_file.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_gui.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_interface.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_list.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_midi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_misc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_net.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_scalar.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_text.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_time.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_vexp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_vexp_if.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/src/x_vexp_fun.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/libpd_wrapper/s_libpdmidi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/libpd_wrapper/x_libpdreceive.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/libpd_wrapper/z_hooks.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/libpd_wrapper/z_libpd.c
)

set (PD_EXTRA_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/extra/bob~/bob~.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/extra/bonk~/bonk~.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/extra/choice/choice.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/extra/fiddle~/fiddle~.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/extra/loop~/loop~.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/extra/lrshift~/lrshift~.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/extra/pique/pique.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/extra/sigmund~/sigmund~.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/pure-data/extra/stdout/stdout.c
)

set (LIBPD_UTILS_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/libpd_wrapper/util/z_print_util.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/libpd_wrapper/util/z_queued.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/libpd_wrapper/util/ringbuffer.c
)

set (LIBPD_HPP_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/cpp/PdBase.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/libpd/cpp/PdTypes.hpp
)

include(CheckIncludeFile)
check_include_file("alloca.h" HAVE_ALLOCA_H_CONFIG)
if(HAVE_ALLOCA_H_CONFIG)
  set(ALLOCA_FLAG "-DHAVE_ALLOCA_H")
endif()

set (LIBPD_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/externals)
set (LIBPD_INCLUDE_DIR ${LIBPD_OUTPUT_DIRECTORY}/include/libpd)

add_custom_target (libpd-headers ALL
  COMMAND cmake -E make_directory ${LIBPD_INCLUDE_DIR}
  COMMAND cmake -E copy libpd/cpp/PdBase.hpp ${LIBPD_INCLUDE_DIR}
  COMMAND cmake -E copy libpd/cpp/PdMidiReceiver.hpp ${LIBPD_INCLUDE_DIR}
  COMMAND cmake -E copy libpd/cpp/PdReceiver.hpp ${LIBPD_INCLUDE_DIR}
  COMMAND cmake -E copy libpd/cpp/PdTypes.hpp ${LIBPD_INCLUDE_DIR}
  COMMAND cmake -E copy libpd/pure-data/src/m_pd.h ${LIBPD_INCLUDE_DIR}
  COMMAND cmake -E copy libpd/libpd_wrapper/z_libpd.h ${LIBPD_INCLUDE_DIR}
  COMMAND cmake -E make_directory ${LIBPD_INCLUDE_DIR}/util
  COMMAND cmake -E copy_directory libpd/libpd_wrapper/util
                                  ${LIBPD_INCLUDE_DIR}/util
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Moving libpd headers"
)

add_library (pdcpp SHARED ${PD_SRC_FILES} ${PD_EXTRA_FILES} ${LIBPD_UTILS_FILES}
                         ${LIBPD_HPP_FILES})


target_include_directories(pdcpp PRIVATE
  libpd/libpd_wrapper
  libpd/pure-data/src
  libpd/libpd_wrapper/util
  libpd/cpp
  libpd/pure-data/extra
)

include(CheckTypeSize)
check_type_size("long" SIZEOF_LONG)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PD_64BIT 1)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|i686|i386")
    set(PD_LITTLE_ENDIAN 1)
    message(STATUS "Forcing little-endian for x86 architecture")
endif()


include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
  set(ENDIAN_DEF "PD_BIG_ENDIAN=1")
else()
  set(ENDIAN_DEF "PD_LITTLE_ENDIAN=1")
endif()

target_compile_definitions(pdcpp PUBLIC
  PD
  PD_INTERNAL
  PD_NET=1
  PD_MULTI_INSTANCE
  HAVE_UNISTD_H
  HAVE_LIBDL
  USEAPI_DUMMY
  HAVE_ALLOCA_H
  ${ENDIAN_DEF}
)

if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  message(STATUS "ENABLE_LIBPD_MULTI is ON: adding PDINSTANCE/PDTHREADS to pdcpp target")
  target_compile_definitions(pdcpp PUBLIC PDINSTANCE=1 PDTHREADS=1)
endif()

target_compile_options(pdcpp PUBLIC -fPIC)

target_link_options(pdcpp PUBLIC "-Wl,-Bsymbolic")

add_dependencies (pdcpp libpd-headers)
target_link_libraries (pdcpp PUBLIC
  dl
  m
  pthread
  nsl
  resolv
)

set_target_properties (
  pdcpp PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${LIBPD_OUTPUT_DIRECTORY}/lib
  ARCHIVE_OUTPUT_DIRECTORY ${LIBPD_OUTPUT_DIRECTORY}/lib
)

###############################################################################

install (TARGETS pdcpp
         EXPORT ${PROJECT_EXPORT}
         DESTINATION lib
         INCLUDES DESTINATION include)

install (DIRECTORY ${CMAKE_BINARY_DIR}/externals/include/libpd
         DESTINATION include)


### PD helper patches! #########################################################

# Where patches should look for PD headers
set (PD_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/externals/include)

## OGGREAD~

# Find Vorbis (and Ogg) package 
find_package (Vorbis)

# Define patch build target for oggread~
add_library (oggread SHARED pdogg/oggread~.c)

# Link with libvorbis and libogg
target_link_libraries (
  oggread 
  PUBLIC
  ${VORBIS_LIBRARIES}
  pdcpp
)

# Set a few properties
#   - Warning flags
#   - Output directory, prefix and suffix
#   - Include directories
set_target_properties (
  oggread PROPERTIES
  COMPILE_FLAGS "-Wall -W -fPIC"
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/patches
  PREFIX ""
  SUFFIX "~.pd_linux"
  INCLUDE_DIRECTORIES "${PD_INCLUDE_DIR};${PD_INCLUDE_DIR}/pd"
)

# Install ODA Panel patches
install (FILES
         ${CMAKE_BINARY_DIR}/patches/oggread~.pd_linux
         ${CMAKE_SOURCE_DIR}/externals/pdogg/oggread~-help.pd
         DESTINATION patches
         CONFIGURATIONS Debug Release)

