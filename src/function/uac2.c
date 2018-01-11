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
#include "usbg/function/uac2.h"

#include <malloc.h>
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

struct usbg_f_uac2 {
	struct usbg_function func;
};

#define UAC2_DEC_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_uac2_attrs, _name),    \
		.get = usbg_get_dec,				        \
		.set = usbg_set_dec,				        \
		.import = usbg_get_config_node_int,	                \
		.export = usbg_set_config_node_int,		        \
	}

static struct {
	const char *name;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} uac2_attr[USBG_F_UAC2_ATTR_MAX] = {
	[USBG_F_UAC2_C_CHMASK] = UAC2_DEC_ATTR(c_chmask),
	[USBG_F_UAC2_C_SRATE] = UAC2_DEC_ATTR(c_srate),
	[USBG_F_UAC2_C_SSIZE] = UAC2_DEC_ATTR(c_ssize),
	[USBG_F_UAC2_P_CHMASK] = UAC2_DEC_ATTR(p_chmask),
	[USBG_F_UAC2_P_SRATE] = UAC2_DEC_ATTR(p_srate),
	[USBG_F_UAC2_P_SSIZE] = UAC2_DEC_ATTR(p_ssize),
};

#undef UAC2_DEC_ATTR

GENERIC_ALLOC_INST(uac2, struct usbg_f_uac2, func);

GENERIC_FREE_INST(uac2, struct usbg_f_uac2, func);

static int uac2_set_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_uac2_set_attrs(usbg_to_uac2_function(f), f_attrs);
}

static int uac2_get_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_uac2_get_attrs(usbg_to_uac2_function(f), f_attrs);
}

static void uac2_cleanup_attrs(struct usbg_function *f, void *f_attrs)
{
	usbg_f_uac2_cleanup_attrs(f_attrs);
}

#ifdef HAS_GADGET_SCHEMES

static int uac2_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_uac2 *af = usbg_to_uac2_function(f);
	union usbg_f_uac2_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_UAC2_ATTR_MIN; i < USBG_F_UAC2_ATTR_MAX; ++i) {
		ret = uac2_attr[i].import(root, uac2_attr[i].name, &val);
		/* node not  found */
		if (ret == 0)
			continue;
		/* error */
		if (ret < 0)
			break;

		ret = usbg_f_uac2_set_attr_val(af, i, val);
		if (ret)
			break;
	}

	return ret;
}

static int uac2_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_uac2 *af = usbg_to_uac2_function(f);
	union usbg_f_uac2_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_UAC2_ATTR_MIN; i < USBG_F_UAC2_ATTR_MAX; ++i) {
		ret = usbg_f_uac2_get_attr_val(af, i, &val);
		if (ret)
			break;

		ret = uac2_attr[i].export(root, uac2_attr[i].name, &val);
		if (ret)
			break;
	}

	return ret;
}

#endif /* HAS_GADGET_SCHEMES */

struct usbg_function_type usbg_f_type_uac2 = {
	.name = "uac2",
	.alloc_inst = uac2_alloc_inst,
	.free_inst = uac2_free_inst,
	.set_attrs = uac2_set_attrs,
	.get_attrs = uac2_get_attrs,
	.cleanup_attrs = uac2_cleanup_attrs,

#ifdef HAS_GADGET_SCHEMES
	.import = uac2_libconfig_import,
	.export = uac2_libconfig_export,
#endif
};

/* API implementation */

usbg_f_uac2 *usbg_to_uac2_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_uac2 ?
		container_of(f, struct usbg_f_uac2, func) : NULL;
}

usbg_function *usbg_from_uac2_function(usbg_f_uac2 *af)
{
	return &af->func;
}

int usbg_f_uac2_get_attrs(usbg_f_uac2 *af,
			  struct usbg_f_uac2_attrs *attrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_UAC2_ATTR_MIN; i < USBG_F_UAC2_ATTR_MAX; ++i) {
		ret = usbg_f_uac2_get_attr_val(af, i,
					       (union usbg_f_uac2_attr_val *)
					       ((char *)attrs
						+ uac2_attr[i].offset));
		if (ret)
			break;
	}

	return ret;

}

int usbg_f_uac2_set_attrs(usbg_f_uac2 *af,
			 const struct usbg_f_uac2_attrs *attrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_UAC2_ATTR_MIN; i < USBG_F_UAC2_ATTR_MAX; ++i) {
		ret = usbg_f_uac2_set_attr_val(af, i,
					       *(union usbg_f_uac2_attr_val *)
					       ((char *)attrs
						+ uac2_attr[i].offset));
		if (ret)
			break;
	}

	return ret;

}

int usbg_f_uac2_get_attr_val(usbg_f_uac2 *af, enum usbg_f_uac2_attr attr,
			    union usbg_f_uac2_attr_val *val)
{
	return uac2_attr[attr].get(af->func.path, af->func.name,
				    uac2_attr[attr].name, val);
}

int usbg_f_uac2_set_attr_val(usbg_f_uac2 *af, enum usbg_f_uac2_attr attr,
			     union usbg_f_uac2_attr_val val)
{
	return uac2_attr[attr].set(af->func.path, af->func.name,
				    uac2_attr[attr].name, &val);
}
