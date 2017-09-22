dnl Autoconfigure input file for hexedit
dnl
dnl Process this file with autoconf to produce a configure script.
dnl

AC_PREREQ(2.50)

AC_INIT(hexedit.c)
AC_CONFIG_HEADERS(config.h)

define(TheVERSION, 1.4.2)

PRODUCT=hexedit
VERSION=TheVERSION
INCL=hexedit.h
SRCS="hexedit.c display.c mark.c page.c file.c interact.c misc.c search.c"
OTHER="COPYING Changes TODO install-sh configure config.h.in hexedit.1 hexedit-TheVERSION.lsm Makefile.in configure.in Makefile-build.in"
AC_SUBST(PRODUCT)
AC_SUBST(VERSION)
AC_SUBST(INCL)
AC_SUBST(SRCS)
AC_SUBST(OTHER)

dnl Enable GNU extensions on systems that have them.
AH_VERBATIM([_GNU_SOURCE],
[/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif])
AC_DEFINE(_GNU_SOURCE)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

dnl Checks for libraries.
AC_CHECK_LIB(curses, initscr, LIBS="$LIBS -lcurses",
  [AC_CHECK_LIB(ncurses, initscr, LIBS="$LIBS -lncurses")]
)
AC_CHECK_LIB(tinfo, keypad, LIBS="$LIBS -ltinfo")
AC_CHECK_FUNC(use_default_colors, 
  AC_DEFINE(HAVE_COLORS, , "Define if you want colored (fruit salad) display option")
)

dnl This test must come as early as possible after the compiler configuration
dnl tests, because the choice of the file model can (in principle) affect
dnl whether functions and headers are available, whether they work, etc.
AC_SYS_LARGEFILE

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_STAT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h unistd.h libgen.h sys/mount.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_CHECK_FUNCS(memcmp memmem memrmem memrchr strdup strerror basename)
if test $ac_cv_func_basename = no; then
  # basename not in the default libraries. See if it's elsewhere
  AC_CHECK_LIB(gen, basename, [AC_DEFINE(HAVE_BASENAME)
    LIBS="$LIBS -lgen"])
fi

AC_CHECK_DECLS([dirname, basename, memrchr, memmem, memrmem], , , 
$ac_includes_default
#if HAVE_LIBGEN_H
#include <libgen.h>
#endif
)

AC_OUTPUT(Makefile Makefile-build hexedit.1)
