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

/**
 * @file gadget-ffs.c
 * @example gadget-ffs.c
 * This is an example of how to create gadget with FunctionFS based functions
 * in two ways. After executing this program gadget will not be enabled
 * because ffs instances should be mounted and both descriptors and strings
 * should be written to ep0.
 * For more details about FunctionFS please refer to FunctionFS documentation
 * in linux kernel repository.
 */

#include <errno.h>
#include <stdio.h>
#include <usbg/usbg.h>

#define VENDOR          0x1d6b
#define PRODUCT         0x0104

int main(void)
{
	usbg_state *s;
	usbg_gadget *g;
	usbg_config *c;
	usbg_function *f_ffs1, *f_ffs2;
	int ret = -EINVAL;
	int usbg_ret;
	usbg_function_attrs f_attrs = {
		.ffs = {
			.dev_name = "my_awesome_dev_name",
		},
	};

	usbg_gadget_attrs g_attrs = {
			0x0200, /* bcdUSB */
			0x00, /* Defined at interface level */
			0x00, /* subclass */
			0x00, /* device protocol */
			0x0040, /* Max allowed packet size */
			VENDOR,
			PRODUCT,
			0x0001, /* Verson of device */
	};

	usbg_gadget_strs g_strs = {
			"0123456789", /* Serial number */
			"Foo Inc.", /* Manufacturer */
			"Bar Gadget" /* Product string */
	};

	usbg_config_strs c_strs = {
			"2xFFS"
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

	usbg_ret = usbg_create_function(g, F_FFS, "my_dev_name", NULL, &f_ffs1);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating ffs1 function\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	/* When NULL is passed as instance name, dev_name take from f_attrs
	   is used as instance name for this function */
	usbg_ret = usbg_create_function(g, F_FFS, NULL, &f_attrs, &f_ffs2);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating ffs2 function\n");
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

	usbg_ret = usbg_add_config_function(c, "some_name_here", f_ffs1);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error adding ffs1\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_add_config_function(c, "some_name_here_too", f_ffs2);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error adding ffs2\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	fprintf(stdout, "2xFFS gadget has been created.\n"
		"Enable it after preparing your functions.\n");

	/*
	 * Here we end up with two created ffs instances but they are not
	 * fully operational. Now we have to do step by step:
	 * 1) Mount both instances:
	 *    $ mount my_dev_name -t functionfs /path/to/mount/dir1
	 *    $ mount my_awesome_dev_name -t functionfs /path/to/mount/dir2
	 *
	 * 2) Run ffs daemons for both instances:
	 *    $ my-ffs-daemon /path/to/mount/dir1
	 *    $ my-ffs-daemon /path/to/mount/dir2
	 *
	 * 3) Enable your gadget:
	 *    $ echo "my_udc_name" > /sys/kernel/config/usb_gadget/g1/UDC
	 */

	ret = 0;

out2:
	usbg_cleanup(s);

out1:
	return ret;
}
