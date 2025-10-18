cmake_minimum_required(VERSION 3.5)
project(vorpal_tools CXX)

add_executable(pd_multi_test pd_multi_test.cpp)

add_executable(pdinstance_test pdinstance_test.cpp)

target_include_directories(pdinstance_test PRIVATE
  ${CMAKE_BINARY_DIR}/externals/include/libpd
  ${CMAKE_BINARY_DIR}/externals/include/libpd/util
  ${CMAKE_SOURCE_DIR}/src/vorpal
)

target_link_libraries(pdinstance_test PRIVATE pdcpp vorpal)

target_compile_features(pdinstance_test PRIVATE cxx_std_11)

if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  target_compile_definitions(pdinstance_test PRIVATE PDINSTANCE=1 PDTHREADS=1)
endif()

target_include_directories(pd_multi_test PRIVATE
  ${CMAKE_BINARY_DIR}/externals/include/libpd
  ${CMAKE_BINARY_DIR}/externals/include/libpd/util
)

target_link_libraries(pd_multi_test PRIVATE pdcpp)

target_compile_features(pd_multi_test PRIVATE cxx_std_11)

if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  target_compile_definitions(pd_multi_test PRIVATE PDINSTANCE=1 PDTHREADS=1)
endif()
