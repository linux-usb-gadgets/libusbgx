/*
 * Copyright (C) 2013 Linaro Limited
 *
 * Matt Porter <matt.porter@linaro.org>
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
#include <gadget/gadget.h>

/**
 * @file gadget-acm-ecm.c
 * @example gadget-acm-ecm.c
 * This is an example of how to create an ACM+ECM gadget device.
 */

#define VENDOR		0x1d6b
#define PRODUCT		0x0104

int main(void)
{
	struct state *s;
	struct gadget *g;
	struct function *f;
	struct config *c;
	struct function *f_acm0, *f_acm1, *f_ecm;
	int ret = -EINVAL;

	s = gadget_init("/config");
	if (!s) {
		fprintf(stderr, "Error on USB gadget init\n");
		goto error1;
	}

	g = gadget_create_gadget(s, "g1", VENDOR, PRODUCT);
	if (!g) {
		fprintf(stderr, "Error on create gadget\n");
		goto error2;
	}
	gadget_set_gadget_serial_number(g, LANG_US_ENG, "0123456789");
	gadget_set_gadget_manufacturer(g, LANG_US_ENG, "Foo Inc.");
	gadget_set_gadget_product(g, LANG_US_ENG, "Bar Gadget");

	f_acm0 = gadget_create_function(g, F_ACM, "usb0");
	if (!f_acm0) {
		fprintf(stderr, "Error creating acm0 function\n");
		goto error2;
	}

	f_acm1 = gadget_create_function(g, F_ACM, "usb1");
	if (!f_acm1) {
		fprintf(stderr, "Error creating acm1 function\n");
		goto error2;
	}

	f_ecm = gadget_create_function(g, F_ECM, "usb0");
	if (!f_ecm) {
		fprintf(stderr, "Error creating ecm function\n");
		goto error2;
	}

	c = gadget_create_config(g, "c.1");
	if (!c) {
		fprintf(stderr, "Error creating config\n");
		goto error2;
	}
	gadget_set_config_string(c, LANG_US_ENG, "CDC 2xACM+ECM");
	gadget_add_config_function(c, "acm.GS0", f_acm0);
	gadget_add_config_function(c, "acm.GS1", f_acm1);
	gadget_add_config_function(c, "ecm.usb0", f_ecm);

	gadget_enable_gadget(g, DEFAULT_UDC);

	return 0;

error2:
	gadget_cleanup(s);

error1:
	return -EINVAL;
}
