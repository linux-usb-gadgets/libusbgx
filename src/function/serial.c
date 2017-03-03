/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "usbg/usbg.h"
#include "usbg/usbg_internal.h"
#include "usbg/function/serial.h"

#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

struct usbg_f_serial {
	struct usbg_function func;
};

GENERIC_ALLOC_INST(serial, struct usbg_f_serial, func);

GENERIC_FREE_INST(serial, struct usbg_f_serial, func);

static int serial_set_attrs(struct usbg_function *f, void *f_attrs)
{
	int port_num = *(int *)f_attrs;

	return port_num ? USBG_ERROR_INVALID_PARAM : USBG_SUCCESS;
}

static int serial_get_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_serial_get_port_num(usbg_to_serial_function(f), f_attrs);
}

static int serial_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	return USBG_SUCCESS;
}

static int serial_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	return USBG_SUCCESS;
}

/* We don' import nor export port_num as it is read only */
#define SERIAL_FUNCTION_OPTS			\
	.alloc_inst = serial_alloc_inst,	\
	.free_inst = serial_free_inst,	        \
	.set_attrs = serial_set_attrs,	        \
	.get_attrs = serial_get_attrs,	        \
	.export = serial_libconfig_export,	\
	.import = serial_libconfig_import

struct usbg_function_type usbg_f_type_acm = {
	.name = "acm",
	SERIAL_FUNCTION_OPTS,
};

struct usbg_function_type usbg_f_type_serial = {
	.name = "gser",
	SERIAL_FUNCTION_OPTS,
};

struct usbg_function_type usbg_f_type_obex = {
	.name = "obex",
	SERIAL_FUNCTION_OPTS,
};

/* API implementation */

usbg_f_serial *usbg_to_serial_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_acm ||
		f->ops == &usbg_f_type_serial ||
		f->ops == &usbg_f_type_obex ?
		container_of(f, struct usbg_f_serial, func) : NULL;
}

usbg_function *usbg_from_serial_function(usbg_f_serial *sf)
{
	return &sf->func;
}

int usbg_f_serial_get_port_num(usbg_f_serial *sf, int *port_num)
{
	return usbg_read_dec(sf->func.path, sf->func.name,
			     "port_num", port_num);
}

