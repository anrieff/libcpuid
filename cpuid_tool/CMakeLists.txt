add_executable(cpuid_tool cpuid_tool.c)
target_link_libraries(cpuid_tool PRIVATE cpuid)

install(TARGETS cpuid_tool
        CONFIGURATIONS Debug
        RUNTIME DESTINATION bin/Debug)
install(TARGETS cpuid_tool
        CONFIGURATIONS Release
        RUNTIME DESTINATION bin/Release)