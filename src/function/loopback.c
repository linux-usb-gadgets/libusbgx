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

struct usbg_f_loopback {
	struct usbg_function func;
};

GENERIC_ALLOC_INST(loopback, struct usbg_f_loopback, func);

GENERIC_FREE_INST(loopback, struct usbg_f_loopback, func);

static int loopback_set_attrs(struct usbg_function *f,
			    const usbg_function_attrs *f_attrs)
{
	int ret = USBG_ERROR_INVALID_PARAM;
	const usbg_f_loopback_attrs *attrs = &f_attrs->attrs.loopback;

	if (f_attrs->header.attrs_type &&
	    f_attrs->header.attrs_type != USBG_F_ATTRS_LOOPBACK)
		goto out;

	ret = usbg_write_dec(f->path, f->name, "buflen", attrs->buflen);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_dec(f->path, f->name, "qlen", attrs->qlen);

 out:
	return ret;
}

static int loopback_get_attrs(struct usbg_function *f,
			    usbg_function_attrs *f_attrs)
{
	int ret;
	const usbg_f_loopback_attrs *attrs = &f_attrs->attrs.loopback;

	ret = usbg_read_dec(f->path, f->name, "buflen", (int *)&(attrs->buflen));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_dec(f->path, f->name, "qlen", (int *)&(attrs->qlen));
	if (ret != USBG_SUCCESS)
		goto out;

	f_attrs->header.attrs_type = USBG_F_ATTRS_LOOPBACK;
out:
	return ret;
}

#ifdef HAS_LIBCONFIG

static int loopback_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	config_setting_t *node;
	usbg_function_attrs f_attrs;
	usbg_f_loopback_attrs *attrs = &f_attrs.attrs.loopback;
	int cfg_ret;
	int ret = USBG_ERROR_NO_MEM;

	ret = loopback_get_attrs(f, &f_attrs);
	if (ret)
		goto out;		

#define ADD_F_LOOPBACK_INT_ATTR(attr, minval)				\
	do { 								\
		if ((int)attrs->attr < minval) {			\
			ret = USBG_ERROR_INVALID_VALUE;			\
			goto out;					\
		}							\
		node = config_setting_add(root, #attr, CONFIG_TYPE_INT);\
		if (!node) 						\
			goto out; 					\
		cfg_ret = config_setting_set_int(node, attrs->attr); 	\
		if (cfg_ret != CONFIG_TRUE) { 				\
			ret = USBG_ERROR_OTHER_ERROR; 			\
			goto out; 					\
		}							\
	} while (0)

	ADD_F_LOOPBACK_INT_ATTR(buflen, 0);
	ADD_F_LOOPBACK_INT_ATTR(qlen, 0);

#undef ADD_F_LOOPBACK_INT_ATTR

	ret = USBG_SUCCESS;
out:
	return ret;
}

static int loopback_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	config_setting_t *node;
	int ret = USBG_ERROR_NO_MEM;
	int tmp;
	usbg_function_attrs attrs;
	usbg_f_loopback_attrs *loopback_attrs = &attrs.attrs.loopback;

	attrs.header.attrs_type = USBG_F_ATTRS_LOOPBACK;

#define ADD_F_LOOPBACK_INT_ATTR(attr, defval, minval)			\
	do {								\
		node = config_setting_get_member(root, #attr);		\
		if (node) {						\
			if (!usbg_config_is_int(node)) {		\
				ret = USBG_ERROR_INVALID_TYPE;		\
				goto out;				\
			}						\
			tmp = config_setting_get_int(node);		\
			if (tmp < minval) {				\
				ret = USBG_ERROR_INVALID_VALUE;		\
				goto out;				\
			}						\
			loopback_attrs->attr = tmp;				\
		} else {						\
			loopback_attrs->attr = defval;			\
		}							\
	} while (0)

	ADD_F_LOOPBACK_INT_ATTR(buflen, 4096, 0);
	ADD_F_LOOPBACK_INT_ATTR(qlen, 32, 0);

#undef ADD_F_LOOPBACK_INT_ATTR

	ret = usbg_set_function_attrs(f, &attrs);
out:
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

