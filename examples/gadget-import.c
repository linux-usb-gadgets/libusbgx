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
 * @file gadget-import.c
 * @example gadget-import.c
 * This is an example of how to import a gadget from file.
 * Common reason of doing this is to create gadget base on schema
 * from other devices or resurect gadget after reboot.
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <usbg/usbg.h>

int main(int argc, char **argv)
{
	usbg_state *s;
	int ret = -EINVAL;
	int usbg_ret;
	FILE *input;

	if (argc != 3) {
		fprintf(stderr, "Usage: gadget-import gadget_name file_name\n");
		return ret;
	}

	/* Prepare input file */
	input = fopen(argv[2], "r");
	if (!input) {
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

	usbg_ret = usbg_import_gadget(s, input, argv[1], NULL);
	if (usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on import gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		if (usbg_ret == USBG_ERROR_INVALID_FORMAT)
			fprintf(stderr, "Line: %d. Error: %s\n",
				usbg_get_gadget_import_error_line(s),
				usbg_get_gadget_import_error_text(s));
		goto out3;
	}

	ret = 0;

out3:
	usbg_cleanup(s);
out2:
	fclose(input);
out1:
	return ret;
}
