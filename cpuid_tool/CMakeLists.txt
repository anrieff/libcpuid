add_executable(cpuid_tool cpuid_tool.c)

target_link_libraries(cpuid_tool cpuid)

if(WIN32)
  install(
    TARGETS cpuid_tool
    CONFIGURATIONS Debug
    RUNTIME DESTINATION bin/Debug)
  install(
    TARGETS cpuid_tool
    CONFIGURATIONS Release
    RUNTIME DESTINATION bin/Release)
else()
  install(TARGETS cpuid_tool DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
