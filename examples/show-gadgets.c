/*
 * Copyright (C) 2013 Linaro Limited
 *
 * Matt Porter <mporter@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ether.h>
#include <usbg/usbg.h>

/**
 * @file show-gadgets.c
 * @example show-gadgets.c
 * This example shows how to display all configured USB gadgets
 * in the system
 */

void show_gadget(usbg_gadget *g)
{
	char buf[USBG_MAX_STR_LENGTH];
	int usbg_ret;
	usbg_gadget_attrs g_attrs;
	usbg_gadget_strs g_strs;

	usbg_get_gadget_name(g, buf, USBG_MAX_STR_LENGTH);
	usbg_ret = usbg_get_gadget_attrs(g, &g_attrs);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		return;
	}

	fprintf(stdout, "ID %04x:%04x '%s'\n",
			g_attrs.idVendor, g_attrs.idProduct, buf);

	usbg_get_gadget_udc(g, buf, USBG_MAX_STR_LENGTH);
	fprintf(stdout, "  UDC\t\t\t%s\n", buf);

	fprintf(stdout, "  bDeviceClass\t\t0x%02x\n", g_attrs.bDeviceClass);
	fprintf(stdout, "  bDeviceSubClass\t0x%02x\n", g_attrs.bDeviceSubClass);
	fprintf(stdout, "  bDeviceProtocol\t0x%02x\n", g_attrs.bDeviceProtocol);
	fprintf(stdout, "  bMaxPacketSize0\t0x%02x\n", g_attrs.bMaxPacketSize0);
	fprintf(stdout, "  bcdDevice\t\t0x%04x\n", g_attrs.bcdDevice);
	fprintf(stdout, "  bcdUSB\t\t0x%04x\n", g_attrs.bcdUSB);
	fprintf(stdout, "  idVendor\t\t0x%04x\n", g_attrs.idVendor);
	fprintf(stdout, "  idProduct\t\t0x%04x\n", g_attrs.idProduct);

	usbg_get_gadget_strs(g, LANG_US_ENG, &g_strs);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		return;
	}
	fprintf(stdout, "  Serial Number\t\t%s\n", g_strs.str_ser);
	fprintf(stdout, "  Manufacturer\t\t%s\n", g_strs.str_mnf);
	fprintf(stdout, "  Product\t\t%s\n", g_strs.str_prd);
}

void show_function(usbg_function *f)
{
	char buf[USBG_MAX_STR_LENGTH];
	int usbg_ret;
	usbg_function_attrs f_attrs;

	usbg_get_function_name(f, buf, USBG_MAX_STR_LENGTH);
	usbg_ret = usbg_get_function_attrs(f, &f_attrs);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		return;
	}

	fprintf(stdout, "  Function '%s'\n", buf);
	switch (usbg_get_function_type(f)) {
	case F_SERIAL:
	case F_ACM:
	case F_OBEX:
		fprintf(stdout, "    port_num\t\t%d\n",
				f_attrs.serial.port_num);
		break;
	case F_ECM:
	case F_SUBSET:
	case F_NCM:
	case F_EEM:
	case F_RNDIS:
		fprintf(stdout, "    dev_addr\t\t%s\n",
				ether_ntoa(&f_attrs.net.dev_addr));
		fprintf(stdout, "    host_addr\t\t%s\n",
				ether_ntoa(&f_attrs.net.host_addr));
		fprintf(stdout, "    ifname\t\t%s\n", f_attrs.net.ifname);
		fprintf(stdout, "    qmult\t\t%d\n", f_attrs.net.qmult);
		break;
	case F_PHONET:
		fprintf(stdout, "    ifname\t\t%s\n", f_attrs.phonet.ifname);
		break;
	default:
		fprintf(stdout, "    UNKNOWN\n");
	}
}

void show_config(usbg_config *c)
{
	usbg_binding *b;
	usbg_function *f;
	char buf[USBG_MAX_STR_LENGTH], buf2[USBG_MAX_STR_LENGTH];
	usbg_config_attrs c_attrs;
	usbg_config_strs c_strs;
	int usbg_ret;

	usbg_get_config_name(c, buf, USBG_MAX_STR_LENGTH);
	fprintf(stdout, "  Configuration '%s'\n", buf);

	usbg_ret = usbg_get_config_attrs(c, &c_attrs);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		return;
	}

	fprintf(stdout, "    MaxPower\t\t%d\n", c_attrs.bMaxPower);
	fprintf(stdout, "    bmAttributes\t0x%02x\n", c_attrs.bmAttributes);

	usbg_ret = usbg_get_config_strs(c, LANG_US_ENG, &c_strs);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		return;
	}

	fprintf(stdout, "    configuration\t%s\n", c_strs.configuration);

	usbg_for_each_binding(b, c) {
		usbg_get_binding_name(b, buf, USBG_MAX_STR_LENGTH);
		f = usbg_get_binding_target(b);
		usbg_get_function_name(f, buf2, USBG_MAX_STR_LENGTH);
		fprintf(stdout, "    %s -> %s\n", buf, buf2);
	}
}

int main(void)
{
	usbg_state *s;
	usbg_gadget *g;
	usbg_function *f;
	usbg_config *c;
	int usbg_ret;

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB gadget init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		return -EINVAL;
	}

	usbg_for_each_gadget(g, s) {
		show_gadget(g);
		usbg_for_each_function(f, g)
			show_function(f);
		usbg_for_each_config(c, g)
			show_config(c);
	}

	usbg_cleanup(s);

	return 0;
}
