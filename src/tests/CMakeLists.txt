# VORPAL Multi-Instance Tests
# All tests for multi-instance support, each with isolated resources

# Test 1: libpd multi-instance compilation verification
add_executable(pd_multi_test 
  pd_multi_test/pd_multi_test.cpp
)
target_include_directories(pd_multi_test PRIVATE
  ${CMAKE_BINARY_DIR}/externals/include/libpd
  ${CMAKE_BINARY_DIR}/externals/include/libpd/util
)
target_link_libraries(pd_multi_test PRIVATE pdcpp)
target_compile_features(pd_multi_test PRIVATE cxx_std_11)
if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  target_compile_definitions(pd_multi_test PRIVATE PDINSTANCE=1 PDTHREADS=1)
endif()

# Test 2: PDInstance isolation stress test (6 instances)
add_executable(pdinstance_test 
  pdinstance_test/pdinstance_test.cpp
)
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

# Test 3: InstanceManager lifecycle and negative tests
add_executable(instancemanager_test 
  instancemanager_test/instancemanager_test.cpp
)
target_include_directories(instancemanager_test PRIVATE
  ${CMAKE_BINARY_DIR}/externals/include/libpd
  ${CMAKE_BINARY_DIR}/externals/include/libpd/util
  ${CMAKE_SOURCE_DIR}/src/vorpal
)
target_link_libraries(instancemanager_test PRIVATE pdcpp vorpal)
target_compile_features(instancemanager_test PRIVATE cxx_std_11)
if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  target_compile_definitions(instancemanager_test PRIVATE PDINSTANCE=1 PDTHREADS=1)
endif()

# Test 4: Auto-ID generation and sequential IDs
add_executable(instancemanager_autoid_test 
  instancemanager_autoid_test/instancemanager_autoid_test.cpp
)
target_include_directories(instancemanager_autoid_test PRIVATE
  ${CMAKE_BINARY_DIR}/externals/include/libpd
  ${CMAKE_BINARY_DIR}/externals/include/libpd/util
  ${CMAKE_SOURCE_DIR}/src/vorpal
)
target_link_libraries(instancemanager_autoid_test PRIVATE pdcpp vorpal)
target_compile_features(instancemanager_autoid_test PRIVATE cxx_std_11)
if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  target_compile_definitions(instancemanager_autoid_test PRIVATE PDINSTANCE=1 PDTHREADS=1)
endif()

# Test 5: Engine API and dangling pointer fix validation
add_executable(engine_multiinstance_test 
  engine_multiinstance_test/engine_multiinstance_test.cpp
)
target_include_directories(engine_multiinstance_test PRIVATE
  ${CMAKE_BINARY_DIR}/externals/include/libpd
  ${CMAKE_BINARY_DIR}/externals/include/libpd/util
  ${CMAKE_SOURCE_DIR}/src/vorpal
)
target_link_libraries(engine_multiinstance_test PRIVATE pdcpp vorpal ${OPENAL_LIBRARY})
target_compile_features(engine_multiinstance_test PRIVATE cxx_std_11)
if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  target_compile_definitions(engine_multiinstance_test PRIVATE PDINSTANCE=1 PDTHREADS=1)
endif()

# Test 6: Command routing isolation (PRIMARY FUNCTIONAL TEST)
add_executable(command_routing_test 
  command_routing_test/command_routing_test.cpp
)
target_include_directories(command_routing_test PRIVATE
  ${CMAKE_BINARY_DIR}/externals/include/libpd
  ${CMAKE_BINARY_DIR}/externals/include/libpd/util
  ${CMAKE_SOURCE_DIR}/src/vorpal
)
target_link_libraries(command_routing_test PRIVATE pdcpp vorpal)
target_compile_features(command_routing_test PRIVATE cxx_std_11)
if(DEFINED ENABLE_LIBPD_MULTI AND ENABLE_LIBPD_MULTI)
  target_compile_definitions(command_routing_test PRIVATE PDINSTANCE=1 PDTHREADS=1)
endif()

# Set output directory for all test executables to bin/
set_target_properties(
  pd_multi_test
  pdinstance_test
  instancemanager_test
  instancemanager_autoid_test
  engine_multiinstance_test
  command_routing_test
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy test patches to build directory - each test creates its own subdirectory
add_custom_command(TARGET pdinstance_test POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/patches/pdinstance_test
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/pdinstance_test/patches
    ${CMAKE_BINARY_DIR}/bin/patches/pdinstance_test
  COMMENT "Copying pdinstance_test patches"
)

add_custom_command(TARGET instancemanager_test POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/patches/instancemanager_test
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/instancemanager_test/patches
    ${CMAKE_BINARY_DIR}/bin/patches/instancemanager_test
  COMMENT "Copying instancemanager_test patches"
)

add_custom_command(TARGET instancemanager_autoid_test POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/patches/instancemanager_autoid_test
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/instancemanager_autoid_test/patches
    ${CMAKE_BINARY_DIR}/bin/patches/instancemanager_autoid_test
  COMMENT "Copying instancemanager_autoid_test patches"
)

add_custom_command(TARGET command_routing_test POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/patches/command_routing_test
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/command_routing_test/patches
    ${CMAKE_BINARY_DIR}/bin/patches/command_routing_test
  COMMENT "Copying command_routing_test patches"
)

# ==============================================================================
# CTest Integration - Automated Test Execution
# ==============================================================================

enable_testing()

# Register all multi-instance tests with CTest
add_test(NAME pd_multi_test 
         COMMAND pd_multi_test 
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_test(NAME pdinstance_test 
         COMMAND pdinstance_test 
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_test(NAME instancemanager_test 
         COMMAND instancemanager_test 
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_test(NAME instancemanager_autoid_test 
         COMMAND instancemanager_autoid_test 
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_test(NAME engine_multiinstance_test 
         COMMAND engine_multiinstance_test 
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_test(NAME command_routing_test 
         COMMAND command_routing_test 
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set test timeouts (30 seconds per test)
set_tests_properties(
  pd_multi_test
  pdinstance_test
  instancemanager_test
  instancemanager_autoid_test
  engine_multiinstance_test
  command_routing_test
  PROPERTIES TIMEOUT 30
)
