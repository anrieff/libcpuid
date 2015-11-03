ACLOCAL_AMFLAGS = -I m4
SUBDIRS = libcpuid cpuid_tool tests

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = libcpuid.pc
EXTRA_DIST     = libcpuid.pc.in libcpuid_vc71.sln libcpuid_vc9.sln Readme.md

consistency:
	$(top_srcdir)/libcpuid/check-consistency.py $(top_srcdir)/libcpuid

test:
	$(top_srcdir)/tests/run_tests.py $(top_srcdir)/cpuid_tool/cpuid_tool $(top_srcdir)/tests

fix-tests:
	$(top_srcdir)/tests/run_tests.py $(top_srcdir)/cpuid_tool/cpuid_tool $(top_srcdir)/tests --fix
