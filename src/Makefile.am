AUTOMAKE_OPTIONS = std-options subdir-objects
lib_LTLIBRARIES = libusbgx.la
libusbgx_la_SOURCES = usbg.c usbg_error.c usbg_common.c function/ether.c function/ffs.c function/midi.c function/ms.c function/phonet.c function/serial.c function/loopback.c function/hid.c function/uac2.c function/uvc.c function/printer.c function/9pfs.c
if TEST_GADGET_SCHEMES
libusbgx_la_SOURCES += usbg_schemes_libconfig.c usbg_common_libconfig.c
else
libusbgx_la_SOURCES += usbg_schemes_none.c
endif
libusbgx_la_LDFLAGS = $(LIBCONFIG_LIBS)
libusbgx_la_LDFLAGS += -version-info 2:0:0
libusbgx_la_CFLAGS = $(LIBCONFIG_CFLAGS)
AM_CPPFLAGS=-I$(top_srcdir)/include/ -I$(top_builddir)/include/usbg
