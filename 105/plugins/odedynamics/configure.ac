#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)

AC_INIT([proto-odedynamics], [1], [proto-bugs@csail.mit.edu])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([../../config])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([foreign])

AC_PROG_CXX
AC_LANG(C++)

## Checks for programs.

AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

PROTO_WITH_ODE
PROTO_WITH_GLUT

if test x"$WANT_ODE" = xfalse; then
   AC_MSG_ERROR([odedyanamics cannot be built without ODE support.])
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
