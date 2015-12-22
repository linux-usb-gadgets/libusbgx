AC_INIT([libusbgx], [0.0.1], [k.opasiak@samsung.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AM_PROG_AR
AC_CONFIG_MACRO_DIR([m4])
AC_DEFINE([_GNU_SOURCE], [], [Use GNU extensions])

AC_ARG_ENABLE([gadget-schemes], AS_HELP_STRING([--disable-gadget-schemes],
				  [build without gadget-schemes support]), [enable_gadget_schemes=$enableval],
							  [enable_gadget_schemes=yes])
if test x$enable_gadget_schemes != xno; then
	PKG_CHECK_MODULES([LIBCONFIG], [libconfig >= 1.4],
			  [ AC_DEFINE(HAS_GADGET_SCHEMES, 1, [detected libconfig])
			    PKG_CHECK_MODULES([NEW_LIBCONFIG], [libconfig >= 1.5],
			     AC_DEFINE(HAVE_LIBCONFIG_15, 1, [detected libconfig equal to or greater than 1.5]),
			     AC_DEFINE(HAVE_LIBCONFIG_15, 0, []))
			  ])
	CFLAGS="$CFLAGS $LIBCONFIG_CFLAGS"
	LIBS="$LIBS $LIBCONFIG_LIBS"
fi
AM_CONDITIONAL(TEST_GADGET_SCHEMES, test x$enable_gadget_schemes != xno )
LT_INIT
AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile libusbgx.pc])
DX_INIT_DOXYGEN([$PACKAGE_NAME],[doxygen.cfg])
AC_OUTPUT
