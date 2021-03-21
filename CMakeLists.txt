cmake_minimum_required(VERSION 3.14)

set(VERSION "0.5.1")
set(SOVERSION 15)
project(
  cpuid
  LANGUAGES C CXX ASM_MASM
  VERSION ${VERSION})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

# Global variables
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(UNIX)
  include(GNUInstallDirs)
  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(exec_prefix "\${prefix}")
  set(libdir "\${exec_prefix}/lib")
  set(includedir "\${prefix}/include")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libcpuid.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/libcpuid.pc" ESCAPE_QUOTES
                 @ONLY)
  install(FILES "${PROJECT_BINARY_DIR}/libcpuid.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif(UNIX)

# Include subdirectories
add_subdirectory(libcpuid)
add_subdirectory(cpuid_tool)
add_subdirectory(tests)
