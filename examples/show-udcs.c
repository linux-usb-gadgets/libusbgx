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
 * @file show-udcs.c
 * @example show-udcs.c
 * This is an example of how to learn about UDCs available in system
 * and find out what gadget are enabled on them.
 */

#include <errno.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <usbg/usbg.h>

int main(void)
{
	int usbg_ret;
	int ret = -EINVAL;
	usbg_state *s;
	usbg_gadget *g;
	usbg_udc *u;
	const char *udc_name, *gadget_name;

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB state init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out;
	}

	usbg_for_each_udc(u, s) {
		udc_name = usbg_get_udc_name(u);
		g = usbg_get_udc_gadget(u);
		if (g)
			/* some gadget is enabled */
			gadget_name = usbg_get_gadget_name(g);
		else
			gadget_name = "";

		fprintf(stdout, "%s <-> %s\n", udc_name, gadget_name);
	}

	ret = 0;
	usbg_cleanup(s);
out:
	return ret;
}
