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
#include "usbg/function/ffs.h"

#include <malloc.h>
#ifdef HAS_LIBCONFIG
#include <libconfig.h>
#endif

struct usbg_f_fs {
	struct usbg_function func;
};

GENERIC_ALLOC_INST(ffs, struct usbg_f_fs, func);

GENERIC_FREE_INST(ffs, struct usbg_f_fs, func);

static int ffs_set_attrs(struct usbg_function *f, void *f_attrs)
{
	const char *dev_name = *(const char **)f_attrs;

	return dev_name && dev_name[0] ? USBG_ERROR_INVALID_PARAM
		: USBG_SUCCESS;
}

static int ffs_get_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_fs_get_dev_name(usbg_to_fs_function(f), f_attrs);
}

static void ffs_cleanup_attrs(struct usbg_function *f, void *f_attrs)
{
	free(*(char **)f_attrs);
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

/* API implementation */

usbg_f_fs *usbg_to_fs_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_ffs ?
		container_of(f, struct usbg_f_fs, func) : NULL;
}

usbg_function *usbg_from_fs_function(usbg_f_fs *ff)
{
	return &ff->func;
}

int usbg_f_fs_get_dev_name(usbg_f_fs *ff, char **dev_name)
{
	if (!ff || !dev_name)
		return USBG_ERROR_INVALID_PARAM;

	*dev_name = strdup(ff->func.instance);
	if (!*dev_name)
		return USBG_ERROR_NO_MEM;

	return 0;
}

int usbg_f_fs_get_dev_name_s(usbg_f_fs *ff, char *buf, int len)
{
	if (!ff || !buf)
		return USBG_ERROR_INVALID_PARAM;

	return snprintf(buf, len, "%s", ff->func.instance);
}

