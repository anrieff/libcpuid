# Include "arm" directory only for ARM CPUs
if(("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^armv.*") OR ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64"))
  add_subdirectory(arm)
endif()
