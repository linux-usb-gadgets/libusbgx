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
 * @file gadget-vid-pid-remove.c
 * @example gadget-vid-pid-remove.c
 * This is an example of how to find and remove an gadget device with given
 * Vendor ID and product ID.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <usbg/usbg.h>

#define VENDOR		0x1d6b
#define PRODUCT		0x0104

int remove_gadget(usbg_gadget *g)
{
	int usbg_ret;
	usbg_udc *u;

	/* Check if gadget is enabled */
	u = usbg_get_gadget_udc(g);

	/* If gadget is enable we have to disable it first */
	if (u) {
		usbg_ret = usbg_disable_gadget(g);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr, "Error on USB disable gadget udc\n");
			fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
					usbg_strerror(usbg_ret));
			goto out;
		}
	}

	/* Remove gadget with USBG_RM_RECURSE flag to remove
	 * also its configurations, functions and strings */
	usbg_ret = usbg_rm_gadget(g, USBG_RM_RECURSE);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB gadget remove\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
	}

out:
	return usbg_ret;
}

int main(int argc, char **argv)
{
	int usbg_ret;
	int ret = -EINVAL;
	usbg_state *s;
	usbg_gadget *g;
	struct usbg_gadget_attrs g_attrs;
	char *cp;
	int vendor = VENDOR, product = PRODUCT;

	if (argc >= 2) {
		cp = strchr(argv[1], ':');
		if (!cp) {
			ret = -EINVAL;
			fprintf(stderr, "Usage: gadget-vid-pid-remove vid:pid\n");
			goto out1;
		}
		*cp++ = 0;
		if (&argv[1])
			vendor = strtoul(argv[1], NULL, 16);
		if (*cp)
			product = strtoul(cp, NULL, 16);
	}

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB state init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out1;
	}

	g = usbg_get_first_gadget(s);
	while (g != NULL) {
		/* Get current gadget attrs to be compared */
		usbg_ret = usbg_get_gadget_attrs(g, &g_attrs);
		if (usbg_ret != USBG_SUCCESS) {
			fprintf(stderr, "Error on USB get gadget attrs\n");
			fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
					usbg_strerror(usbg_ret));
			goto out2;
		}

		/* Compare attrs with given values and remove if suitable */
		if (g_attrs.idVendor == vendor && g_attrs.idProduct == product) {
			usbg_gadget *g_next = usbg_get_next_gadget(g);

			usbg_ret = remove_gadget(g);
			if (usbg_ret != USBG_SUCCESS)
				goto out2;

			g = g_next;
		} else {
			g = usbg_get_next_gadget(g);
		}
	}

out2:
	usbg_cleanup(s);
out1:
	return ret;
}
