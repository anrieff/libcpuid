SUBDIRS = libcpuid cpuid_tool tests

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = libcpuid.pc
EXTRA_DIST     = libcpuid.pc.in libcpuid.sln

consistency:
	$(top_srcdir)/libcpuid/check-consistency.py $(top_srcdir)/libcpuid

test:
	$(top_srcdir)/tests/run_tests.py $(top_srcdir)/tests/tests_stash.txt $(top_srcdir)/cpuid_tool/cpuid_tool
