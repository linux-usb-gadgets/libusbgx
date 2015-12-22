include $(top_srcdir)/aminclude.am
SUBDIRS = src examples

if BUILD_TESTS
SUBDIRS += tests
endif

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = doxygen.cfg
library_includedir=$(includedir)/usbg
library_include_HEADERS = include/usbg/usbg.h
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libusbgx.pc
