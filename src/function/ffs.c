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

struct usbg_f_fs {
	struct usbg_function func;
};

GENERIC_ALLOC_INST(ffs, struct usbg_f_fs, func);

GENERIC_FREE_INST(ffs, struct usbg_f_fs, func);

static int ffs_set_attrs(struct usbg_function *f,
			 const usbg_function_attrs *f_attrs)
{
	const usbg_f_ffs_attrs *ffs_attrs = &(f_attrs->attrs.ffs);
	int ret = USBG_ERROR_INVALID_PARAM;

	if (f_attrs->header.attrs_type &&
	    f_attrs->header.attrs_type != USBG_F_ATTRS_FFS)
		goto out;

	ret = ffs_attrs->dev_name && ffs_attrs->dev_name[0] ?
		USBG_ERROR_INVALID_PARAM : USBG_SUCCESS;

out:
	return ret;
}

static int ffs_get_attrs(struct usbg_function *f,
			 usbg_function_attrs *f_attrs)
{
	usbg_f_ffs_attrs *ffs_attrs = &(f_attrs->attrs.ffs);
	int ret = USBG_SUCCESS;

	ffs_attrs->dev_name = strdup(f->instance);
	if (!ffs_attrs->dev_name) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	f_attrs->header.attrs_type = USBG_F_ATTRS_FFS;
out:
	return ret;
}

static void ffs_cleanup_attrs(struct usbg_function *f,
			      usbg_function_attrs *f_attrs)
{
	free((char*)f_attrs->attrs.ffs.dev_name);
	f_attrs->attrs.ffs.dev_name = NULL;
}

static int ffs_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	return USBG_SUCCESS;
}

static int ffs_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	return USBG_SUCCESS;
}

struct usbg_function_type usbg_f_type_ffs = {
	.name = "ffs",
	.alloc_inst = ffs_alloc_inst,
	.free_inst = ffs_free_inst,
	.set_attrs = ffs_set_attrs,
	.get_attrs = ffs_get_attrs,
	.cleanup_attrs = ffs_cleanup_attrs,
	.import = ffs_libconfig_import,
	.export = ffs_libconfig_export,
};
