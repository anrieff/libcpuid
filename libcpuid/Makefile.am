EXTRA_DIST =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =

lib_LTLIBRARIES += libcpuid.la
libcpuid_la_LDFLAGS = \
	-export-symbols $(srcdir)/libcpuid.sym \
	-no-undefined -version-info @LIBCPUID_VERSION_INFO@
libcpuid_la_SOURCES =		\
	cpuid_main.c		\
	recog_intel.c		\
	recog_amd.c		\
	rdtsc.c			\
	asm-bits.c		\
	libcpuid_util.c		\
	rdmsr.c

libcpuid_la_DEPENDENCIES = \
	$(srcdir)/libcpuid.sym

libcpuidincludedir = $(includedir)/libcpuid
libcpuidinclude_HEADERS = 	\
	libcpuid.h		\
	libcpuid_constants.h	\
	libcpuid_types.h

noinst_HEADERS = 		\
	asm-bits.h		\
	libcpuid_util.h		\
	recog_intel.h		\
	recog_amd.h		\
	rdtsc.h

EXTRA_DIST += libcpuid.sym libcpuid.vcproj
