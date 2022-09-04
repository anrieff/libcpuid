add_custom_target(test DEPENDS test-fast)

add_custom_target(
  test-fast
  COMMAND ${CMAKE_COMMAND} -E env "LD_LD_PRELOAD=${CMAKE_BINARY_DIR}/libcpuid" ./run_tests.py
          "${CMAKE_BINARY_DIR}/cpuid_tool/cpuid_tool" "." --show-test-fast-warning
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMENT "Run tests (fast mode)"
  VERBATIM)

add_custom_target(
  test-old
  COMMAND ${CMAKE_COMMAND} -E env "LD_LD_PRELOAD=${CMAKE_BINARY_DIR}/libcpuid" ./run_tests.py
          "${CMAKE_BINARY_DIR}/cpuid_tool/cpuid_tool" "."
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMENT "Run tests (old mode)"
  VERBATIM)

add_custom_target(
  fix-tests
  COMMAND ${CMAKE_COMMAND} -E env "LD_LD_PRELOAD=${CMAKE_BINARY_DIR}/libcpuid" ./run_tests.py
          "${CMAKE_BINARY_DIR}/cpuid_tool/cpuid_tool" "." --fix
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMENT "Fix tests"
  VERBATIM)
