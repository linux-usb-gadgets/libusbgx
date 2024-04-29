/*
 * Copyright (C) 2024 Pengutronix
 *
 * Michael Grzeschik <m.grzeschik@pengutronix.de>
 *
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
#include "usbg/function/9pfs.h"

#include <malloc.h>
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

struct usbg_f_9pfs {
	struct usbg_function func;
};

GENERIC_ALLOC_INST(p9fs, struct usbg_f_9pfs, func);

GENERIC_FREE_INST(p9fs, struct usbg_f_9pfs, func);

static int p9fs_set_attrs(struct usbg_function *f, void *f_attrs)
{
	const char *dev_name = *(const char **)f_attrs;

	return dev_name && dev_name[0] ? USBG_ERROR_INVALID_PARAM
		: USBG_SUCCESS;
}

static int p9fs_get_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_9pfs_get_dev_name(usbg_to_9pfs(f), f_attrs);
}

static void p9fs_cleanup_attrs(struct usbg_function *f, void *f_attrs)
{
	free(*(char **)f_attrs);
}

static int p9fs_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	return USBG_SUCCESS;
}

static int p9fs_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	return USBG_SUCCESS;
}

struct usbg_function_type usbg_f_type_9pfs = {
	.name = "usb9pfs",
	.alloc_inst = p9fs_alloc_inst,
	.free_inst = p9fs_free_inst,
	.set_attrs = p9fs_set_attrs,
	.get_attrs = p9fs_get_attrs,
	.cleanup_attrs = p9fs_cleanup_attrs,
	.import = p9fs_libconfig_import,
	.export = p9fs_libconfig_export,
};

/* API implementation */

usbg_f_9pfs *usbg_to_9pfs(usbg_function *f)
{
	return f->ops == &usbg_f_type_9pfs ?
		container_of(f, struct usbg_f_9pfs, func) : NULL;
}

usbg_function *usbg_from_9pfs(usbg_f_9pfs *p9fs)
{
	return &p9fs->func;
}

int usbg_f_9pfs_get_dev_name(usbg_f_9pfs *p9fs, char **dev_name)
{
	if (!p9fs || !dev_name)
		return USBG_ERROR_INVALID_PARAM;

	*dev_name = strdup(p9fs->func.instance);
	if (!*dev_name)
		return USBG_ERROR_NO_MEM;

	return 0;
}

int usbg_f_9pfs_get_dev_name_s(usbg_f_9pfs *p9fs, char *buf, int len)
{
	if (!p9fs || !buf)
		return USBG_ERROR_INVALID_PARAM;

	return snprintf(buf, len, "%s", p9fs->func.instance);
}

