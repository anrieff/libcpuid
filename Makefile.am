SUBDIRS = libcpuid cpuid_tool

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = libcpuid.pc
EXTRA_DIST     = libcpuid.pc.in

consistency:
	$(top_srcdir)/libcpuid/check-consistency.py $(top_srcdir)/libcpuid
