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

#ifdef HAS_LIBCONFIG
#include <libconfig.h>
#endif

struct usbg_f_serial {
	struct usbg_function func;
};

GENERIC_ALLOC_INST(serial, struct usbg_f_serial, func);

GENERIC_FREE_INST(serial, struct usbg_f_serial, func);

static int serial_set_attrs(struct usbg_function *f,
			    const usbg_function_attrs *f_attrs)
{
	int ret = USBG_ERROR_INVALID_PARAM;

	if (f_attrs->header.attrs_type &&
	    f_attrs->header.attrs_type != USBG_F_ATTRS_SERIAL)
		goto out;

	ret = f_attrs->attrs.serial.port_num ?
		USBG_ERROR_INVALID_PARAM : USBG_SUCCESS;
 out:
	return ret;
}

static int serial_get_attrs(struct usbg_function *f,
			    usbg_function_attrs *f_attrs)
{
	int ret;

	ret = usbg_read_dec(f->path, f->name, "port_num",
				&(f_attrs->attrs.serial.port_num));
	if (ret != USBG_SUCCESS)
		goto out;

	f_attrs->header.attrs_type = USBG_F_ATTRS_SERIAL;
out:
	return ret;
}

#ifdef HAS_LIBCONFIG

static int serial_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	config_setting_t *node;
	usbg_function_attrs f_attrs;
	usbg_f_serial_attrs *attrs = &f_attrs.attrs.serial;
	int cfg_ret;
	int ret = USBG_ERROR_NO_MEM;

	ret = serial_get_attrs(f, &f_attrs);
	if (ret)
		goto out;		

	node = config_setting_add(root, "port_num", CONFIG_TYPE_INT);
	if (!node) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	cfg_ret = config_setting_set_int(node, attrs->port_num);
	ret = cfg_ret == CONFIG_TRUE ? 0 : USBG_ERROR_OTHER_ERROR;

out:
	return ret;
}

#define SERIAL_LIBCONFIG_DEP_OPS		\
	.export = serial_libconfig_export

#else

#define SERIAL_LIBCONFIG_DEP_OPS

#endif /* HAS_LIBCONFIG */

static int serial_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	return USBG_SUCCESS;
}

/* We don' import port_num as it is read only */
#define SERIAL_FUNCTION_OPTS			\
	.alloc_inst = serial_alloc_inst,	\
	.free_inst = serial_free_inst,	        \
	.set_attrs = serial_set_attrs,	        \
	.get_attrs = serial_get_attrs,	        \
	SERIAL_LIBCONFIG_DEP_OPS,	        \
	.import = serial_libconfig_import

struct usbg_function_type usbg_f_type_acm = {
	.name = "acm",
	SERIAL_FUNCTION_OPTS
};

struct usbg_function_type usbg_f_type_serial = {
	.name = "gser",
	SERIAL_FUNCTION_OPTS
};

struct usbg_function_type usbg_f_type_obex = {
	.name = "obex",
	SERIAL_FUNCTION_OPTS
};
