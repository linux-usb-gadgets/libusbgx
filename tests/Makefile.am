check_PROGRAMS = test header_checker
test_SOURCES = test.c usbg-test.c usbg-io-wrappers.c
test_LDFLAGS = -ldl
test_LDFLAGS += $(CMOCKA_LIBS)
test_LDFLAGS += $(LIBCONFIG_LIBS)
test_LDADD = ./libusbgx.so
test_CPPFLAGS = -I$(top_srcdir)/include/

./libusbgx.so:
	-ln -s $(top_srcdir)/src/.libs/libusbgx.so* .
CLEANFILES = libusbgx.so*

header_checker_SOURCES = header_checker.cpp
header_checker_LDFLAGS = -ldl
header_checker_LDFLAGS += $(CMOCKA_LIBS)
header_checker_LDFLAGS += $(LIBCONFIG_LIBS)
header_checker_LDADD = ./libusbgx.so
header_checker_CPPFLAGS = -I$(top_srcdir)/include/

check_SCRIPTS = ./test.sh
TESTS = $(check_SCRIPTS) header_checker
