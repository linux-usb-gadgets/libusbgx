/*
 * Copyright (C) 2014 Samsung Electronics
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
#include <stdint.h>
#include <stdio.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/hid.h>

#define VENDOR          0x1d6b
#define PRODUCT         0x0104

static uint8_t report_desc[] = {
	0x05, 0x01,	/* USAGE_PAGE (Generic Desktop)	          */
	0x09, 0x06,	/* USAGE (Keyboard)                       */
	0xa1, 0x01,	/* COLLECTION (Application)               */
	0x05, 0x07,	/*   USAGE_PAGE (Keyboard)                */
	0x19, 0xe0,	/*   USAGE_MINIMUM (Keyboard LeftControl) */
	0x29, 0xe7,	/*   USAGE_MAXIMUM (Keyboard Right GUI)   */
	0x15, 0x00,	/*   LOGICAL_MINIMUM (0)                  */
	0x25, 0x01,	/*   LOGICAL_MAXIMUM (1)                  */
	0x75, 0x01,	/*   REPORT_SIZE (1)                      */
	0x95, 0x08,	/*   REPORT_COUNT (8)                     */
	0x81, 0x02,	/*   INPUT (Data,Var,Abs)                 */
	0x95, 0x01,	/*   REPORT_COUNT (1)                     */
	0x75, 0x08,	/*   REPORT_SIZE (8)                      */
	0x81, 0x03,	/*   INPUT (Cnst,Var,Abs)                 */
	0x95, 0x05,	/*   REPORT_COUNT (5)                     */
	0x75, 0x01,	/*   REPORT_SIZE (1)                      */
	0x05, 0x08,	/*   USAGE_PAGE (LEDs)                    */
	0x19, 0x01,	/*   USAGE_MINIMUM (Num Lock)             */
	0x29, 0x05,	/*   USAGE_MAXIMUM (Kana)                 */
	0x91, 0x02,	/*   OUTPUT (Data,Var,Abs)                */
	0x95, 0x01,	/*   REPORT_COUNT (1)                     */
	0x75, 0x03,	/*   REPORT_SIZE (3)                      */
	0x91, 0x03,	/*   OUTPUT (Cnst,Var,Abs)                */
	0x95, 0x06,	/*   REPORT_COUNT (6)                     */
	0x75, 0x08,	/*   REPORT_SIZE (8)                      */
	0x15, 0x00,	/*   LOGICAL_MINIMUM (0)                  */
	0x25, 0x65,	/*   LOGICAL_MAXIMUM (101)                */
	0x05, 0x07,	/*   USAGE_PAGE (Keyboard)                */
	0x19, 0x00,	/*   USAGE_MINIMUM (Reserved)             */
	0x29, 0x65,	/*   USAGE_MAXIMUM (Keyboard Application) */
	0x81, 0x00,	/*   INPUT (Data,Ary,Abs)                 */
	0xc0		/* END_COLLECTION                         */
};

int main() {
	usbg_state *s;
	usbg_gadget *g;
	usbg_config *c;
	usbg_function *f_hid;
	int ret = -EINVAL;
	int usbg_ret;

	struct usbg_gadget_attrs g_attrs = {
		.bcdUSB = 0x0200,
		.bDeviceClass =	USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass = 0x00,
		.bDeviceProtocol = 0x00,
		.bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
		.idVendor = VENDOR,
		.idProduct = PRODUCT,
		.bcdDevice = 0x0001, /* Verson of device */
	};

	struct usbg_gadget_strs g_strs = {
		.serial = "0123456789", /* Serial number */
		.manufacturer = "Foo Inc.", /* Manufacturer */
		.product = "Bar Gadget" /* Product string */
	};

	struct usbg_config_strs c_strs = {
		.configuration = "1xHID"
	};

	struct usbg_f_hid_attrs f_attrs = {
		.protocol = 1,
		.report_desc = {
			.desc = (char *)report_desc,
			.len = sizeof(report_desc),
		},
		.report_length = 8,
		.subclass = 0,
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
	usbg_ret = usbg_create_function(g, USBG_F_HID, "usb0", &f_attrs, &f_hid);
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

	usbg_ret = usbg_add_config_function(c, "some_name", f_hid);
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
