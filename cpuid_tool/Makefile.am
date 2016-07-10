bin_PROGRAMS = cpuid_tool

cpuid_tool_SOURCES = cpuid_tool.c

EXTRA_INCLUDE_PATHS = -I$(top_srcdir)/libcpuid
AM_CPPFLAGS = $(all_includes) $(EXTRA_INCLUDE_PATHS)

cpuid_tool_LDADD = $(top_builddir)/libcpuid/libcpuid.la

EXTRA_DIST = cpuid_tool_vc71.vcproj cpuid_tool_vc10.vcxproj cpuid_tool_vc10.vcxproj.filters
