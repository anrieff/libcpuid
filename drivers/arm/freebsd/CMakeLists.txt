set(DRIVER_NAME "cpuid")
set(DRIVER_SOURCE "${DRIVER_NAME}.c")
set(DRIVER_OBJECT "${DRIVER_NAME}.o")
set(DRIVER_MODULE "${DRIVER_NAME}.ko")

configure_file("${DRIVER_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_SOURCE}" COPYONLY)
configure_file(Makefile.in "${CMAKE_CURRENT_BINARY_DIR}/Makefile_kmod")

add_custom_command(OUTPUT "${DRIVER_MODULE}"
  COMMAND "${CMAKE_MAKE_PROGRAM}" -f Makefile_kmod
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS "${DRIVER_SOURCE}"
  COMMENT "Building CPUID kernel module for ARM..."
  VERBATIM
)

add_custom_target(driver-arm-cpuid ALL DEPENDS "${DRIVER_MODULE}")
