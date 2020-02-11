cmake_minimum_required(VERSION 3.14)

set(VERSION "0.4.1")
project(cpuid LANGUAGES C CXX ASM_MASM VERSION ${VERSION})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

add_subdirectory(libcpuid)
add_subdirectory(cpuid_tool)