cmake_minimum_required(VERSION 3.5)
project(vorpal_tools CXX)

add_executable(pd_multi_test pd_multi_test.cpp)

target_include_directories(pd_multi_test PRIVATE
  ${CMAKE_BINARY_DIR}/externals/include/libpd
  ${CMAKE_BINARY_DIR}/externals/include/libpd/util
)

target_link_libraries(pd_multi_test PRIVATE pdcpp)

target_compile_features(pd_multi_test PRIVATE cxx_std_11)

if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  target_compile_definitions(pd_multi_test PRIVATE PDINSTANCE=1 PDTHREADS=1)
endif()
