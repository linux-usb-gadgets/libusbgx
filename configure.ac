AC_INIT([libusbgx], [0.0.1], [k.opasiak@samsung.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AM_PROG_AR
AC_CONFIG_MACRO_DIR([m4])
AC_DEFINE([_GNU_SOURCE], [], [Use GNU extensions])
PKG_CHECK_MODULES(LIBCONFIG, libconfig)
LT_INIT
AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile libusbgx.pc])
DX_INIT_DOXYGEN([$PACKAGE_NAME],[doxygen.cfg])
AC_OUTPUT
