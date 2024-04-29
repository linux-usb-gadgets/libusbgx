include $(top_srcdir)/aminclude.am
SUBDIRS = src

if BUILD_EXAMPLES
SUBDIRS += examples
endif

if BUILD_TESTS
SUBDIRS += tests
endif

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = doxygen.cfg
library_includedir=$(includedir)/usbg
library_include_HEADERS = include/usbg/usbg.h include/usbg/usbg_version.h
function_includedir=$(includedir)/usbg/function
function_include_HEADERS = include/usbg/function/ffs.h include/usbg/function/loopback.h include/usbg/function/midi.h include/usbg/function/ms.h include/usbg/function/net.h include/usbg/function/phonet.h include/usbg/function/serial.h include/usbg/function/hid.h include/usbg/function/uac2.h include/usbg/function/uvc.h include/usbg/function/printer.h include/usbg/function/9pfs.h
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libusbgx.pc
cmakeconfigdir = $(libdir)/cmake/LibUsbgx
cmakeconfig_DATA = LibUsbgxConfig.cmake
