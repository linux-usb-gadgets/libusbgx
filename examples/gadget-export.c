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
 * @file gadget-export.c
 * @example gadget-export.c
 * This is an example of how to export a gadget to file.
 * Common reason of doing this is to share schema of gadget
 * between different devices or preserve gadget between reboots.
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <usbg/usbg.h>

int main(int argc, char **argv)
{
	usbg_state *s;
	usbg_gadget *g;
	int ret = -EINVAL;
	int usbg_ret;
	FILE *output;

	if (argc != 3) {
		fprintf(stderr, "Usage: gadget-export gadget_name file_name\n");
		return ret;
	}

	/* Prepare output file */
	output = fopen(argv[2], "w");
	if (!output) {
		fprintf(stderr, "Error on fopen. Error: %s\n", strerror(errno));
		goto out1;
	}

	/* Do gadget exporting */
	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB gadget init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	g = usbg_get_gadget(s, argv[1]);
	if (!g) {
		fprintf(stderr, "Error on get gadget\n");
		goto out3;
	}

	usbg_ret = usbg_export_gadget(g, output);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on export gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out3;
	}

	ret = 0;

out3:
	usbg_cleanup(s);
out2:
	fclose(output);
out1:
	return ret;
}
