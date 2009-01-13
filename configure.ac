AC_INIT([libcpuid CPU Identification library], [0.1.0], [libcpuid-devel@lists.sourceforge.net], [libcpuid])
AC_CONFIG_SRCDIR([libcpuid/libcpuid.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall dist-bzip2 dist-zip foreign])

dnl Versioning scheme shamelessly copied from libexif.
dnl C:A:R
dnl 10:0:0   Version 0.1.0
LIBCPUID_CURRENT=10
LIBCPUID_AGE=0
LIBCPUID_REVISION=0
AC_SUBST([LIBCPUID_AGE])
AC_SUBST([LIBCPUID_REVISION])
AC_SUBST([LIBCPUID_CURRENT])
AC_SUBST([LIBCPUID_CURRENT_MIN],[`expr $LIBCPUID_CURRENT - $LIBCPUID_AGE`])
LIBCPUID_VERSION_INFO="$LIBCPUID_CURRENT:$LIBCPUID_REVISION:$LIBCPUID_AGE"
AC_SUBST([LIBCPUID_VERSION_INFO])


AC_PROG_CC
AC_C_CONST
AM_PROG_LIBTOOL
AM_CPPFLAGS="$CPPFLAGS"

AC_CHECK_HEADERS([stdint.h])

AM_LDFLAGS="$LDFLAGS"
if test "x$GCC" = "xyes"; then
    AM_CFLAGS="$AM_CFLAGS -ansi -pedantic-error"
    AM_CXXFLAGS="$AM_CXXFLAGS -ansi -pedantic-error"
    AM_CPPFLAGS="$AM_CPPFLAGS -g -Wall -Wmissing-declarations -Wmissing-prototypes"
    AM_LDFLAGS="$AM_LDFLAGS -g -Wall"
fi


AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)


AC_CONFIG_FILES([
  Makefile
  libcpuid.pc
  libcpuid/Makefile
  cpuid_tool/Makefile
  tests/Makefile
])
AC_OUTPUT

