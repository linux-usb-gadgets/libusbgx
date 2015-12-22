/*
 * Copyright (C) 2013 Linaro Limited
 *
 * Matt Porter <mporter@linaro.org>
 *
 * Copyright (C) 2013 - 2015 Samsung Electronics
 *
 * Krzysztof Opasiak <k.opasiak@samsung.com>
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
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>

/**
 * @file gadget-acm-ecm.c
 * @example gadget-acm-ecm.c
 * This is an example of how to create an ACM+ECM gadget device.
 */

#define VENDOR		0x1d6b
#define PRODUCT		0x0104

int main(void)
{
	usbg_state *s;
	usbg_gadget *g;
	usbg_config *c;
	usbg_function *f_acm0, *f_acm1, *f_ecm;
	int ret = -EINVAL;
	int usbg_ret;

	usbg_gadget_attrs g_attrs = {
		.bcdUSB = 0x0200,
		.bDeviceClass =	USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass = 0x00,
		.bDeviceProtocol = 0x00,
		.bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
		.idVendor = VENDOR,
		.idProduct = PRODUCT,
		.bcdDevice = 0x0001, /* Verson of device */
	};

	usbg_gadget_strs g_strs = {
		.str_ser = "0123456789", /* Serial number */
		.str_mnf = "Foo Inc.", /* Manufacturer */
		.str_prd = "Bar Gadget" /* Product string */
	};

	usbg_config_strs c_strs = {
		.configuration = "CDC 2xACM+ECM"
	};

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB gadget init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out1;
	}

	usbg_ret = usbg_create_gadget(s, "g1", &g_attrs, &g_strs, &g);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on create gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_create_function(g, F_ACM, "usb0", NULL, &f_acm0);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating acm0 function\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_create_function(g, F_ACM, "usb1", NULL, &f_acm1);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating acm1 function\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_create_function(g, F_ECM, "usb0", NULL, &f_ecm);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating ecm function\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	/* NULL can be passed to use kernel defaults */
	usbg_ret = usbg_create_config(g, 1, "The only one", NULL, &c_strs, &c);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating config\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_add_config_function(c, "acm.GS0", f_acm0);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error adding acm.GS0\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_add_config_function(c, "acm.GS1", f_acm1);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error adding acm.GS1\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_add_config_function(c, "ecm.usb0", f_ecm);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error adding ecm.usb0\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_enable_gadget(g, DEFAULT_UDC);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error enabling gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	ret = 0;

out2:
	usbg_cleanup(s);

out1:
	return ret;
}
