AC_INIT([libusbgx], [0.2.0], [k.opasiak@samsung.com])
AM_INIT_AUTOMAKE([1.12 -Wall -Werror foreign serial-tests])
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR
AM_PROG_CC_C_O
AC_CONFIG_MACRO_DIR([m4])
AC_DEFINE([_GNU_SOURCE], [], [Use GNU extensions])

USBG_VERSION_HEX=`printf '0x%02x%02x%04x' $(printf '%s' "$PACKAGE_VERSION" | sed -e 's/\./ /g')`
AC_SUBST([USBG_VERSION_HEX])

AC_ARG_WITH([libconfig],
	    AS_HELP_STRING([--without-libconfig], [build without using libconfig]),
	                   [with_libconfig=$withval], [with_libconfig=yes])

AC_ARG_ENABLE([examples],
	      AS_HELP_STRING([--disable-examples], [build without examples]),
	      [enable_examples=$enableval], [enable_examples=yes])

AC_ARG_ENABLE([gadget-schemes],
	      AS_HELP_STRING([--disable-gadget-schemes], [build without gadget-schemes support]),
	      [enable_gadget_schemes=$enableval], [enable_gadget_schemes=yes])

AC_ARG_ENABLE([tests],
	      AS_HELP_STRING([--enable-tests], [build with tests]),
	      [enable_tests=$enableval], [enable_tests=no])

# if both tests and schemes are disabled, we do not need libconfig
AS_IF([test "x$enable_gadget_schemes" = xno && test "x$enable_tests" = xno], [with_libconfig=no])

AS_IF([test "x$with_libconfig" = xyes], [
	PKG_CHECK_MODULES([LIBCONFIG], [libconfig >= 1.4],
			  [ AC_DEFINE(HAS_LIBCONFIG, 1, [detected libconfig])
			    PKG_CHECK_MODULES([NEW_LIBCONFIG], [libconfig >= 1.5],
			    AC_DEFINE(HAVE_LIBCONFIG_15, 1, [detected libconfig equal to or greater than 1.5]),
			    AC_DEFINE(HAVE_LIBCONFIG_15, 0, []))
			  ])
	libconfig_req="libconfig"
	CFLAGS="$CFLAGS $LIBCONFIG_CFLAGS"
	LIBS="$LIBS $LIBCONFIG_LIBS"
], [
	libconfig_req=""
	enable_gadget_schemes=no
])

REQUIRES="$libconfig_req"

AC_SUBST([REQUIRES])

AM_CONDITIONAL(BUILD_EXAMPLES, [test "x$enable_examples" = xyes])

AS_IF([test "x$enable_tests" = xyes], [
	PKG_CHECK_MODULES([CMOCKA], [cmocka >= 1.0.0],
			  AC_DEFINE(HAS_CMOCKA, 1, [detected cmocka]))
])
AM_CONDITIONAL(BUILD_TESTS, [test "x$enable_tests" = xyes])

AS_IF([test "x$enable_gadget_schemes" = xyes],
	[AC_DEFINE(HAS_GADGET_SCHEMES, 1, [gadget schemes are enables])])
AM_CONDITIONAL(TEST_GADGET_SCHEMES, [test "x$enable_gadget_schemes" != xno])

LT_INIT
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile examples/Makefile include/usbg/usbg_version.h libusbgx.pc doxygen.cfg LibUsbgxConfig.cmake])
DX_INIT_DOXYGEN([$PACKAGE_NAME],[doxygen.cfg])
AC_OUTPUT
