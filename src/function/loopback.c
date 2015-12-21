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
#include "usbg/function/loopback.h"

#ifdef HAS_LIBCONFIG
#include <libconfig.h>
#endif

struct usbg_f_loopback {
	struct usbg_function func;
};

static const char *loopback_attr_names[USBG_F_LOOPBACK_ATTR_MAX] = {
	[USBG_F_LOOPBACK_BUFLEN] = "buflen",
	[USBG_F_LOOPBACK_QLEN] = "qlen",
};

static size_t loopback_offsets[USBG_F_LOOPBACK_ATTR_MAX] = {
	[USBG_F_LOOPBACK_BUFLEN] = offsetof(struct usbg_f_loopback_attrs,
					    buflen),
	[USBG_F_LOOPBACK_QLEN] = offsetof(struct usbg_f_loopback_attrs, qlen),
};

GENERIC_ALLOC_INST(loopback, struct usbg_f_loopback, func);

GENERIC_FREE_INST(loopback, struct usbg_f_loopback, func);

static int loopback_set_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_loopback_set_attrs(usbg_to_loopback_function(f), f_attrs);
}

static int loopback_get_attrs(struct usbg_function *f, void *f_attrs)
{

	return usbg_f_loopback_get_attrs(usbg_to_loopback_function(f), f_attrs);
}

#ifdef HAS_LIBCONFIG

static int loopback_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_loopback *lf = usbg_to_loopback_function(f);
	config_setting_t *node;
	int i, tmp, cfg_ret;
	int ret = 0;

	for (i = USBG_F_LOOPBACK_ATTR_MIN; i < USBG_F_LOOPBACK_ATTR_MAX; ++i) {
		ret = usbg_f_loopback_get_attr_val(lf, i, &tmp);
		if (ret)
			break;

		if (tmp < 0) {
			ret = USBG_ERROR_INVALID_VALUE;
			break;
		}
		node = config_setting_add(root, loopback_attr_names[i],
					  CONFIG_TYPE_INT);
		if (!node) {
			ret = USBG_ERROR_NO_MEM;
			break;
		}

		cfg_ret = config_setting_set_int(node, tmp);
		if (cfg_ret != CONFIG_TRUE) {
			ret = USBG_ERROR_OTHER_ERROR;
			break;
		}
	}

	return ret;
}

static int loopback_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_loopback *lf = usbg_to_loopback_function(f);
	config_setting_t *node;
	int i, tmp;
	int ret = 0;

	for (i = USBG_F_LOOPBACK_ATTR_MIN; i < USBG_F_LOOPBACK_ATTR_MAX; ++i) {
		node = config_setting_get_member(root, loopback_attr_names[i]);
		if (!node)
			continue;

		tmp = config_setting_get_int(node);
		if (tmp < 0) {
			ret = USBG_ERROR_INVALID_PARAM;
			break;
		}

		ret = usbg_f_loopback_set_attr_val(lf, i, tmp);
		if (ret)
			break;
	}

	return ret;
}

#endif /* HAS_LIBCONFIG */

struct usbg_function_type usbg_f_type_loopback = {
	.name = "loopback",
	.alloc_inst = loopback_alloc_inst,
	.free_inst = loopback_free_inst,
	.set_attrs = loopback_set_attrs,
	.get_attrs = loopback_get_attrs,
#ifdef HAS_LIBCONFIG
	.import = loopback_libconfig_import,
	.export = loopback_libconfig_export,
#endif
};

/* API implementation */

usbg_f_loopback *usbg_to_loopback_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_loopback ?
		container_of(f, struct usbg_f_loopback, func) : NULL;
}

usbg_function *usbg_from_loopback_function(usbg_f_loopback *lf)
{
	return &lf->func;
}

int usbg_f_loopback_get_attrs(usbg_f_loopback *lf,
			      struct usbg_f_loopback_attrs *attrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_LOOPBACK_ATTR_MIN; i < USBG_F_LOOPBACK_ATTR_MAX; ++i) {
		ret = usbg_f_loopback_get_attr_val(lf,
						   i,
						   (int *)((char *)attrs
						    + loopback_offsets[i]));
		if (ret)
			break;
	}

	return ret;
}

int usbg_f_loopback_set_attrs(usbg_f_loopback *lf,
			 const struct usbg_f_loopback_attrs *attrs)
{
	int i;
	int ret;

	for (i = USBG_F_LOOPBACK_ATTR_MIN; i < USBG_F_LOOPBACK_ATTR_MAX; ++i) {
		ret = usbg_f_loopback_set_attr_val(lf,
						   i,
						   *(int *)((char *)attrs
						     + loopback_offsets[i]));
		if (ret)
			break;
	}

	return ret;
}

int usbg_f_loopback_get_attr_val(usbg_f_loopback *lf,
				 enum usbg_f_loopback_attr attr, int *val)
{
	return usbg_read_dec(lf->func.path, lf->func.name,
			     loopback_attr_names[attr], val);
}

int usbg_f_loopback_set_attr_val(usbg_f_loopback *lf,
				 enum usbg_f_loopback_attr attr, int val)
{
	return usbg_write_dec(lf->func.path, lf->func.name,
			     loopback_attr_names[attr], val);
}

