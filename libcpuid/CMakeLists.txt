set(cpuid_sources cpuid_main.c
    recog_intel.c
    recog_amd.c
    rdtsc.c
    libcpuid_util.c
    rdmsr.c
    msrdriver.c
    asm-bits.c)

if ("${MSVC_CXX_ARCHITECTURE_ID}" MATCHES "x64")
    list(APPEND cpuid_sources masm-x64.asm)
endif ()

add_library(cpuid
            ${cpuid_sources}
            )
target_include_directories(cpuid SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

target_compile_definitions(cpuid PRIVATE VERSION="${PROJECT_VERSION}")
set(CPUID_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/libcpuid.h ${CMAKE_CURRENT_SOURCE_DIR}/libcpuid_constants.h ${CMAKE_CURRENT_SOURCE_DIR}/libcpuid_types.h)
set_target_properties(cpuid PROPERTIES
                      PUBLIC_HEADER "${CPUID_HEADERS}"
                      )

#Documentation
find_package(Doxygen)
option(ENABLE_DOCS "Enable building documentation." ON)
if (DOXYGEN_FOUND AND ENABLE_DOCS)
    set(top_srcdir ${PROJECT_SOURCE_DIR})
    configure_file(Doxyfile.in
                   Doxyfile
                   ESCAPE_QUOTES
                   )
    add_custom_target(docs
                      ALL
                      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/
                      COMMENT "Generating API documentation with Doxygen" VERBATIM
                      )
else ()
    add_custom_target(docs
                      echo "Doxygen was not installed when CMake was run or ENABLE_DOCS was OFF. Check that Doxygen is installed and rerun `cmake .`" VERBATIM
                      )
endif ()

#Configuration
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Use:
#   * PROJECT_VERSION
write_basic_package_version_file(
        "${version_config}" COMPATIBILITY SameMajorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * TARGETS_EXPORT_NAME
#   * PROJECT_NAME
configure_package_config_file(
        "${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in"
        "${project_config}"
        INSTALL_DESTINATION "${config_install_dir}"
)
#Installation
install(TARGETS cpuid
        EXPORT "${TARGETS_EXPORT_NAME}"
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib"
        RUNTIME DESTINATION "bin"
        INCLUDES DESTINATION "include")

install(FILES "${project_config}" "${version_config}"
        DESTINATION "${config_install_dir}"
        )
install(EXPORT "${TARGETS_EXPORT_NAME}"
        NAMESPACE "${namespace}"
        DESTINATION "${config_install_dir}"
        )