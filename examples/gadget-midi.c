/*
 * Copyright (C) 2014 Samsung Electronics
 *
 * Pawel Szewczyk <p.szewczyk@samsung.com>
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
#include <usbg/function/midi.h>

#define VENDOR          0x1d6b
#define PRODUCT         0x0104

int main() {
	usbg_state *s;
	usbg_gadget *g;
	usbg_config *c;
	usbg_function *f_midi;
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
		.configuration = "1xMIDI"
	};

	struct usbg_f_midi_attrs f_attrs = {
		.index = 0,
		.id = "usb0",
		.buflen = 128,
		.qlen = 16,
		.in_ports = 2,
		.out_ports = 3,
	};

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on usbg init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out1;
	}

	usbg_ret = usbg_create_gadget(s, "g1", &g_attrs, &g_strs, &g);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}
	usbg_ret = usbg_create_function(g, USBG_F_MIDI, "usb0", &f_attrs, &f_midi);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating function\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_create_config(g, 1, "The only one", NULL, &c_strs, &c);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating config\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_add_config_function(c, "some_name", f_midi);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error adding function\n");
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
