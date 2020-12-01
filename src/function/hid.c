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
#include "usbg/function/hid.h"

#include <malloc.h>
#include <assert.h>
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

struct usbg_f_hid {
	struct usbg_function func;
};

#define HID_DEC_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.ro = false,						\
		.offset = offsetof(struct usbg_f_hid_attrs, _name),    \
		.get = usbg_get_dec,				        \
		.set = usbg_set_dec,				        \
		.import = usbg_get_config_node_int,	                \
		.export = usbg_set_config_node_int,		        \
	}

#define HID_DEV_ATTR_RO(_name)						\
	{								\
		.name = #_name,						\
		.ro = true,						\
		.offset = offsetof(struct usbg_f_hid_attrs, _name),    \
		.get = usbg_get_dev,				        \
		.export = usbg_set_config_node_dev,		        \
	}

static int hid_get_report(const char *path, const char *name, const char *attr,
			  void *val)
{
	struct usbg_f_hid_report_desc *report_desc = val;
	char buf[USBG_MAX_FILE_SIZE];
	int ret;

	ret = usbg_read_buf(path, name, attr, buf);
	if (ret < 0)
		return ret;

	report_desc->len = ret;
	if (ret == 0) {
		report_desc->desc = NULL;
	} else {
		report_desc->desc = malloc(report_desc->len);
		if (!report_desc->desc)
			return USBG_ERROR_NO_MEM;
		memcpy(report_desc->desc, buf, report_desc->len);
	}

	return 0;
}

static int hid_set_report(const char *path, const char *name, const char *attr,
			  void *val)
{
	struct usbg_f_hid_report_desc *report_desc = val;
	char *buf = report_desc->desc;
	int len = report_desc->len;
	int ret;

	if (len == 0) {
		buf = "";
		len = 1;
	}

	ret = usbg_write_buf(path, name, attr, buf, len);
	if (ret > 0)
		ret = USBG_SUCCESS;

	return ret;
}

#ifdef HAS_GADGET_SCHEMES
static int hid_get_config_node_report(config_setting_t *root,
				      const char *node_name, void *val)
{
	struct usbg_f_hid_report_desc *report_desc = val;
	char *buf;
	int tmp, i;
	config_setting_t *list_node, *node;
	int len;
	int ret;

	list_node = config_setting_get_member(root, node_name);
	if (!list_node)
		return 0;

	if (!config_setting_is_list(list_node))
		return USBG_ERROR_INVALID_TYPE;

	len = config_setting_length(list_node);

	buf = malloc(len * sizeof(char));
	if (!buf)
		return USBG_ERROR_NO_MEM;

	for (i = 0; i < len; ++i) {
		node = config_setting_get_elem(list_node, i);
		assert(node);

		if (!usbg_config_is_int(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto free_buf;
		}

		tmp = config_setting_get_int(node);
		if (tmp < 0 || tmp > 255) {
			ret = USBG_ERROR_INVALID_VALUE;
			goto free_buf;
		}
		buf[i] = (char)tmp;
	}

	report_desc->desc = buf;
	report_desc->len = len;

	return 1;

free_buf:
	free(buf);
	return ret;
}

static int hid_set_config_node_report(config_setting_t *root,
				      const char *node_name, void *val)
{
	struct usbg_f_hid_report_desc *report_desc = val;
	config_setting_t *node;
	int i;
	int ret = 0;

	node = config_setting_add(root, node_name, CONFIG_TYPE_LIST);
	if (!node)
		return USBG_ERROR_NO_MEM;

	for (i = 0; i < report_desc->len; ++i) {
		int tmp = report_desc->desc[i];
		ret = usbg_set_config_node_int_hex(node, NULL, &tmp);
		if (ret)
			return ret;
	}

	return 0;
}
#else

#define hid_get_config_node_report	NULL
#define hid_set_config_node_report	NULL

#endif

#define HID_RD_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.ro = false,						\
		.offset = offsetof(struct usbg_f_hid_attrs, _name),    \
		.get = hid_get_report,				        \
		.set = hid_set_report,				        \
		.import = hid_get_config_node_report,		        \
		.export = hid_set_config_node_report,		        \
	}

struct {
	const char *name;
	bool ro;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} hid_attr[USBG_F_HID_ATTR_MAX] = {
	[USBG_F_HID_DEV] = HID_DEV_ATTR_RO(dev),
	[USBG_F_HID_PROTOCOL] = HID_DEC_ATTR(protocol),
	[USBG_F_HID_REPORT_DESC] = HID_RD_ATTR(report_desc),
	[USBG_F_HID_REPORT_LENGTH] = HID_DEC_ATTR(report_length),
	[USBG_F_HID_SUBCLASS] = HID_DEC_ATTR(subclass),
};

#undef HID_DEC_ATTR
#undef HID_DEV_ATTR_RO
#undef HID_RD_ATTR

GENERIC_ALLOC_INST(hid, struct usbg_f_hid, func);

GENERIC_FREE_INST(hid, struct usbg_f_hid, func);

static int hid_set_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_hid_set_attrs(usbg_to_hid_function(f), f_attrs);
}

static int hid_get_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_hid_get_attrs(usbg_to_hid_function(f), f_attrs);
}

static void hid_cleanup_attrs(struct usbg_function *f, void *f_attrs)
{
	usbg_f_hid_cleanup_attrs(f_attrs);
}

#ifdef HAS_GADGET_SCHEMES

static int hid_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_hid *hf = usbg_to_hid_function(f);
	union usbg_f_hid_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_HID_ATTR_MIN; i < USBG_F_HID_ATTR_MAX; ++i) {
		if (hid_attr[i].ro)
			continue;

		ret = hid_attr[i].import(root, hid_attr[i].name, &val);
		/* node not  found */
		if (ret == 0)
			continue;
		/* error */
		if (ret < 0)
			break;

		ret = usbg_f_hid_set_attr_val(hf, i, val);
		if (ret)
			break;
	}

	return ret;
}

static int hid_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_hid *hf = usbg_to_hid_function(f);
	union usbg_f_hid_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_HID_ATTR_MIN; i < USBG_F_HID_ATTR_MAX; ++i) {
		ret = usbg_f_hid_get_attr_val(hf, i, &val);
		if (ret)
			break;

		ret = hid_attr[i].export(root, hid_attr[i].name, &val);
		if (ret)
			break;
	}

	return ret;
}

#endif /* HAS_GADGET_SCHEMES */

struct usbg_function_type usbg_f_type_hid = {
	.name = "hid",
	.alloc_inst = hid_alloc_inst,
	.free_inst = hid_free_inst,
	.set_attrs = hid_set_attrs,
	.get_attrs = hid_get_attrs,
	.cleanup_attrs = hid_cleanup_attrs,

#ifdef HAS_GADGET_SCHEMES
	.import = hid_libconfig_import,
	.export = hid_libconfig_export,
#endif
};

/* API implementation */

usbg_f_hid *usbg_to_hid_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_hid ?
		container_of(f, struct usbg_f_hid, func) : NULL;
}

usbg_function *usbg_from_hid_function(usbg_f_hid *hf)
{
	return &hf->func;
}

int usbg_f_hid_get_attrs(usbg_f_hid *hf,
			  struct usbg_f_hid_attrs *attrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_HID_ATTR_MIN; i < USBG_F_HID_ATTR_MAX; ++i) {
		ret = usbg_f_hid_get_attr_val(hf, i,
					       (union usbg_f_hid_attr_val *)
					       ((char *)attrs
						+ hid_attr[i].offset));
		if (ret)
			break;
	}

	return ret;

}

int usbg_f_hid_set_attrs(usbg_f_hid *hf,
			 const struct usbg_f_hid_attrs *attrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_HID_ATTR_MIN; i < USBG_F_HID_ATTR_MAX; ++i) {
		if (hid_attr[i].ro)
			continue;

		ret = usbg_f_hid_set_attr_val(hf, i,
					       *(union usbg_f_hid_attr_val *)
					       ((char *)attrs
						+ hid_attr[i].offset));
		if (ret)
			break;
	}

	return ret;

}

int usbg_f_hid_get_attr_val(usbg_f_hid *hf, enum usbg_f_hid_attr attr,
			    union usbg_f_hid_attr_val *val)
{
	return hid_attr[attr].get(hf->func.path, hf->func.name,
				  hid_attr[attr].name, val);
}

int usbg_f_hid_set_attr_val(usbg_f_hid *hf, enum usbg_f_hid_attr attr,
			    union usbg_f_hid_attr_val val)
{
	return hid_attr[attr].ro ?
		USBG_ERROR_INVALID_PARAM :
		hid_attr[attr].set(hf->func.path, hf->func.name,
				  hid_attr[attr].name, &val);
}

