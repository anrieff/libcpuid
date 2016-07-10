ACLOCAL_AMFLAGS = -I m4
SUBDIRS = libcpuid cpuid_tool tests

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = libcpuid.pc
EXTRA_DIST     = libcpuid.pc.in libcpuid_vc71.sln libcpuid_vc10.sln Readme.md

consistency:
	$(top_srcdir)/libcpuid/check-consistency.py $(top_srcdir)/libcpuid

# "make test" is a bit hacked in order to speed up tests by bypassing the
# libtool wrapper script. The old (slower) version is available as "test-old"

test: test-fast

test-fast:
	LD_PRELOAD=$(top_builddir)/libcpuid/.libs/libcpuid.so $(top_srcdir)/tests/run_tests.py $(top_builddir)/cpuid_tool/.libs/cpuid_tool --show-test-fast-warning $(top_srcdir)/tests

test-old:
	$(top_srcdir)/tests/run_tests.py $(top_builddir)/cpuid_tool/cpuid_tool $(top_srcdir)/tests

fix-tests:
	$(top_srcdir)/tests/run_tests.py $(top_builddir)/cpuid_tool/cpuid_tool $(top_srcdir)/tests --fix
