# Find the kernel release
execute_process(
  COMMAND uname -r
  OUTPUT_VARIABLE KERNEL_RELEASE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Kernel release: ${KERNEL_RELEASE}")

# Find the headers
set(KERNELHEADERS_DIR "/usr/lib/modules/${KERNEL_RELEASE}/build")
if(IS_DIRECTORY "${KERNELHEADERS_DIR}")
  message(STATUS "Kernel headers: ${KERNELHEADERS_DIR}")
  file(GLOB KERNELHEADERS_ARCH_INCLUDE_DIR LIST_DIRECTORIES true "${KERNELHEADERS_DIR}/arch/*/include")
  set(KERNELHEADERS_INCLUDE_DIRS
    "${KERNELHEADERS_DIR}/include"
    "${KERNELHEADERS_ARCH_INCLUDE_DIR}"
    CACHE PATH "Kernel headers include dirs"
  )
  set(KERNELHEADERS_FOUND 1 CACHE STRING "Set to 1 if kernel headers were found")
else()
  set(KERNELHEADERS_FOUND 0 CACHE STRING "Set to 1 if kernel headers were found")
endif()

mark_as_advanced(KERNELHEADERS_FOUND)
