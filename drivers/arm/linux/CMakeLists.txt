set(DRIVER_NAME "cpuid")
option(LIBCPUID_DRIVER_ARM_LINUX_DKMS "Use DKMS for CPUID kernel module for ARM" ON)

if(LIBCPUID_DRIVER_ARM_LINUX_DKMS)
  message(STATUS "Deploying DKMS configuration for CPUID kernel module...")
  set(LIBCPUID_SRC_DIR "/usr/src/${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}")

  configure_file(dkms.conf.in "${CMAKE_CURRENT_BINARY_DIR}/dkms.conf")
  configure_file(Makefile.in  "${CMAKE_CURRENT_BINARY_DIR}/Makefile_dkms")
  install(FILES
    cpuid.c
    "${CMAKE_SOURCE_DIR}/libcpuid/libcpuid_arm_driver.h"
    "${CMAKE_CURRENT_BINARY_DIR}/dkms.conf"
    DESTINATION "${LIBCPUID_SRC_DIR}/"
  )
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Makefile_dkms"
    DESTINATION "${LIBCPUID_SRC_DIR}"
    RENAME "Makefile"
  )
else(LIBCPUID_DRIVER_ARM_LINUX_DKMS)
  find_package(LinuxKernelHeaders REQUIRED)

  set(DRIVER_SOURCE "${DRIVER_NAME}.c")
  set(DRIVER_OBJECT "${DRIVER_NAME}.o")
  set(DRIVER_MODULE "${DRIVER_NAME}.ko")
  set(KBUILD_CMD "${CMAKE_MAKE_PROGRAM}" EXTRA_CFLAGS="-I${CMAKE_SOURCE_DIR}/libcpuid" -C "${KERNELHEADERS_DIR}" "src=${CMAKE_CURRENT_SOURCE_DIR}" "M=${CMAKE_CURRENT_BINARY_DIR}")

  add_custom_command(OUTPUT "${DRIVER_MODULE}"
    COMMAND ${KBUILD_CMD} modules
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${DRIVER_SOURCE}"
    COMMENT "Building CPUID kernel module for ARM..."
    VERBATIM
  )

  add_custom_target(driver-arm-cpuid ALL DEPENDS "${DRIVER_MODULE}")
endif(LIBCPUID_DRIVER_ARM_LINUX_DKMS)
