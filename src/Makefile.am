lib_LTLIBRARIES = libusbgx.la
libusbgx_la_SOURCES = usbg.c usbg_error.c 
if TEST_GADGET_SCHEMES
libusbgx_la_SOURCES += usbg_schemes_libconfig.c
else
libusbgx_la_SOURCES += usbg_schemes_none.c
endif
libusbgx_la_LDFLAGS = $(LIBCONFIG_LIBS)
libusbgx_la_LDFLAGS += -version-info 0:0:0
libusbgx_la_CFLAGS = $(LIBCONFIG_CFLAGS)
AM_CPPFLAGS=-I$(top_srcdir)/include/
