cmake_minimum_required(VERSION 3.13)

set(VERSION "0.7.0")
set(LIBCPUID_CURRENT 17)
set(LIBCPUID_AGE 0)
set(LIBCPUID_REVISION 0)
project(
  cpuid
  LANGUAGES C ASM_MASM
  VERSION ${VERSION})

# CMake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if(MSVC)
  set(LIBCPUID_SHARED OFF)
else()
  set(LIBCPUID_SHARED ON)
endif()

# Options
option(BUILD_SHARED_LIBS "Build building shared libraries" ${LIBCPUID_SHARED})
option(LIBCPUID_BUILD_DRIVERS "Enable building kernel drivers" ON)
option(LIBCPUID_ENABLE_TESTS "Enable tests targets" OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

# pthreads library
if(${CMAKE_SYSTEM_NAME} STREQUAL "DragonFly" OR ${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD")
  find_package(Threads REQUIRED)
endif()

# check if popcount64 is available
include(CheckSymbolExists)
check_symbol_exists(popcount64 "string.h" HAVE_POPCOUNT64)
if(HAVE_POPCOUNT64)
  add_definitions(-DHAVE_POPCOUNT64)
endif(HAVE_POPCOUNT64)

# check if auxiliary vector is available
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  check_symbol_exists(getauxval "sys/auxv.h" HAVE_GETAUXVAL)
  if(HAVE_GETAUXVAL)
    add_definitions(-DHAVE_GETAUXVAL)
  endif(HAVE_GETAUXVAL)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  check_symbol_exists(elf_aux_info "sys/auxv.h" HAVE_ELF_AUX_INFO)
  if(HAVE_ELF_AUX_INFO)
    add_definitions(-DHAVE_ELF_AUX_INFO)
  endif(HAVE_ELF_AUX_INFO)
endif()

# Global variables
if(UNIX)
  include(GNUInstallDirs)
  set(prefix "${CMAKE_INSTALL_PREFIX}")
  set(exec_prefix "\${prefix}")
  set(libdir "\${exec_prefix}/lib")
  set(includedir "\${prefix}/include")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wuninitialized -Wstrict-prototypes -Wformat -Wformat-security -Wunused-parameter -Wdeprecated-declarations")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libcpuid.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/libcpuid.pc" ESCAPE_QUOTES
                 @ONLY)
  install(FILES "${PROJECT_BINARY_DIR}/libcpuid.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif(UNIX)

# Include subdirectories
add_subdirectory(libcpuid)
add_subdirectory(cpuid_tool)
if(LIBCPUID_BUILD_DRIVERS)
  add_subdirectory(drivers)
endif(LIBCPUID_BUILD_DRIVERS)
if(LIBCPUID_ENABLE_TESTS)
  add_subdirectory(tests)
endif(LIBCPUID_ENABLE_TESTS)
