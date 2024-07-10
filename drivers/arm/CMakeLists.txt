# Options
option(LIBCPUID_DRIVER_DEBUG "Debug kernel driver" OFF)

if(LIBCPUID_DRIVER_DEBUG)
  add_definitions(-DLIBCPUID_DRIVER_DEBUG)
endif(LIBCPUID_DRIVER_DEBUG)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
  add_subdirectory(freebsd)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  add_subdirectory(linux)
endif()
