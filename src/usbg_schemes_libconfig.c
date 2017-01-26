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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>

#include "usbg/usbg_internal.h"

#define USBG_NAME_TAG "name"
#define USBG_ATTRS_TAG "attrs"
#define USBG_STRINGS_TAG "strings"
#define USBG_OS_DESCS_TAG "os_descs"
#define USBG_FUNCTIONS_TAG "functions"
#define USBG_CONFIGS_TAG "configs"
#define USBG_LANG_TAG "lang"
#define USBG_TYPE_TAG "type"
#define USBG_INSTANCE_TAG "instance"
#define USBG_ID_TAG "id"
#define USBG_FUNCTION_TAG "function"
#define USBG_INTERFACE_TAG "interface"
#define USBG_CONFIG_ID_TAG "config_id"
#define USBG_TAB_WIDTH 4

static inline int generate_function_label(usbg_function *f, char *buf, int size)
{
	return snprintf(buf, size, "%s_%s",
			 usbg_get_function_type_str(f->type), f->instance);

}

static int usbg_export_binding(usbg_binding *b, config_setting_t *root)
{
	config_setting_t *node;
	int ret = USBG_ERROR_NO_MEM;
	int cfg_ret;
	char label[USBG_MAX_NAME_LENGTH];
	int nmb;

#define CRETAE_ATTR_STRING(SOURCE, NAME)				\
	do {								\
		node = config_setting_add(root, NAME, CONFIG_TYPE_STRING); \
		if (!node)						\
			goto out;					\
		cfg_ret = config_setting_set_string(node, SOURCE);	\
		if (cfg_ret != CONFIG_TRUE) {				\
			ret = USBG_ERROR_OTHER_ERROR;			\
			goto out;					\
		}							\
	} while (0)

	CRETAE_ATTR_STRING(b->name, USBG_NAME_TAG);

	nmb = generate_function_label(b->target, label, sizeof(label));
	if (nmb >= sizeof(label)) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	CRETAE_ATTR_STRING(label, USBG_FUNCTION_TAG);

#undef CRETAE_ATTR_STRING

	ret = USBG_SUCCESS;

out:
	return ret;
}

static int usbg_export_config_bindings(usbg_config *c, config_setting_t *root)
{
	usbg_binding *b;
	config_setting_t *node;
	int ret = USBG_SUCCESS;

	TAILQ_FOREACH(b, &c->bindings, bnode) {
		node = config_setting_add(root, NULL, CONFIG_TYPE_GROUP);
		if (!node) {
			ret = USBG_ERROR_NO_MEM;
			break;
		}

		ret = usbg_export_binding(b, node);
		if (ret != USBG_SUCCESS)
			break;
	}

	return ret;
}

static int usbg_export_config_strs_lang(usbg_config *c, int lang,
					config_setting_t *root)
{
	config_setting_t *node;
	struct usbg_config_strs strs;
	int usbg_ret, cfg_ret;
	int ret = USBG_ERROR_NO_MEM;

	usbg_ret = usbg_get_config_strs(c, lang, &strs);
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, USBG_LANG_TAG, CONFIG_TYPE_INT);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_format(node, CONFIG_FORMAT_HEX);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	cfg_ret = config_setting_set_int(node, lang);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	node = config_setting_add(root, "configuration" , CONFIG_TYPE_STRING);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_string(node, strs.configuration);

	usbg_free_config_strs(&strs);

	ret = cfg_ret == CONFIG_TRUE ? USBG_SUCCESS : USBG_ERROR_OTHER_ERROR;
out:
	return ret;

}

static int usbg_export_config_strings(usbg_config *c, config_setting_t *root)
{
	config_setting_t *node;
	int usbg_ret = USBG_SUCCESS;
	int i;
	int *langs;

	usbg_ret = usbg_get_config_strs_langs(c, &langs);
	if (usbg_ret != USBG_SUCCESS)
		goto out;

	for (i = 0; langs[i]; ++i) {
		node = config_setting_add(root, NULL, CONFIG_TYPE_GROUP);
		if (!node) {
			usbg_ret = USBG_ERROR_NO_MEM;
			break;
		}

		usbg_ret = usbg_export_config_strs_lang(c, langs[i], node);
		if (usbg_ret != USBG_SUCCESS)
			break;
	}

	free(langs);
out:
	return usbg_ret;
}

static int usbg_export_config_attrs(usbg_config *c, config_setting_t *root)
{
	config_setting_t *node;
	struct usbg_config_attrs attrs;
	int usbg_ret, cfg_ret;
	int ret = USBG_ERROR_NO_MEM;

	usbg_ret = usbg_get_config_attrs(c, &attrs);
	if (usbg_ret) {
		ret = usbg_ret;
		goto out;
	}

#define ADD_CONFIG_ATTR(attr_name)					\
	do {								\
		node = config_setting_add(root, #attr_name, CONFIG_TYPE_INT); \
		if (!node)						\
			goto out;					\
		cfg_ret = config_setting_set_format(node, CONFIG_FORMAT_HEX); \
		if (cfg_ret != CONFIG_TRUE) {				\
			ret = USBG_ERROR_OTHER_ERROR;			\
			goto out;					\
		}							\
		cfg_ret = config_setting_set_int(node, attrs.attr_name); \
		if (cfg_ret != CONFIG_TRUE) {				\
			ret = USBG_ERROR_OTHER_ERROR;			\
			goto out;					\
		}							\
	} while (0)

	ADD_CONFIG_ATTR(bmAttributes);
	ADD_CONFIG_ATTR(bMaxPower);

#undef ADD_CONFIG_ATTR

	ret = USBG_SUCCESS;
out:
	return ret;

}

/* This function does not export configuration id because it is more of
 * a property of gadget which contains this config than config itself */
static int usbg_export_config_prep(usbg_config *c, config_setting_t *root)
{
	config_setting_t *node;
	int ret = USBG_ERROR_NO_MEM;
	int usbg_ret;
	int cfg_ret;

	node = config_setting_add(root, USBG_NAME_TAG, CONFIG_TYPE_STRING);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_string(node, c->label);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	node = config_setting_add(root, USBG_ATTRS_TAG, CONFIG_TYPE_GROUP);
	if (!node)
		goto out;

	usbg_ret = usbg_export_config_attrs(c, node);
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, USBG_STRINGS_TAG, CONFIG_TYPE_LIST);
	if (!node)
		goto out;

	usbg_ret = usbg_export_config_strings(c, node);
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, USBG_FUNCTIONS_TAG, CONFIG_TYPE_LIST);
	if (!node)
		goto out;

	ret = usbg_export_config_bindings(c, node);
out:
	return ret;

}

static int usbg_export_gadget_configs(usbg_gadget *g, config_setting_t *root)
{
	usbg_config *c;
	config_setting_t *node, *id_node;
	int ret = USBG_SUCCESS;
	int cfg_ret;

	TAILQ_FOREACH(c, &g->configs, cnode) {
		node = config_setting_add(root, NULL, CONFIG_TYPE_GROUP);
		if (!node) {
			ret = USBG_ERROR_NO_MEM;
			break;
		}

		id_node = config_setting_add(node, USBG_ID_TAG,
					     CONFIG_TYPE_INT);
		if (!id_node) {
			ret = USBG_ERROR_NO_MEM;
			break;
		}

		cfg_ret = config_setting_set_int(id_node, c->id);
		if (cfg_ret != CONFIG_TRUE) {
			ret = USBG_ERROR_OTHER_ERROR;
			break;
		}

		ret = usbg_export_config_prep(c, node);
		if (ret != USBG_SUCCESS)
			break;
	}

	return ret;
}

static int usbg_export_function_attrs(usbg_function *f, config_setting_t *root)
{
	int ret = USBG_ERROR_NOT_SUPPORTED;

	if (!f->ops->export)
		goto out;

	ret = f->ops->export(f, root);
out:
	return ret;
}

static int usbg_export_function_os_descs(usbg_function *f,
					 config_setting_t *root)
{
	config_setting_t *groot;
	config_setting_t *node;
	int cfg_ret;
	int ret;
	struct usbg_function_os_desc f_os_desc = {0};
	const char *iname = f->ops->os_desc_iname;

	ret = usbg_get_interf_os_desc(f, iname, &f_os_desc);
	if (ret)
		goto out;

	groot = config_setting_add(root, NULL, CONFIG_TYPE_GROUP);
	if (!groot)
		goto out;

	node = config_setting_add(groot, USBG_INTERFACE_TAG, CONFIG_TYPE_STRING);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_string(node, iname);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	ret = usbg_set_config_node_os_desc(groot, iname, &f_os_desc);

out:
	usbg_free_interf_os_desc(&f_os_desc);
	return ret;
}

/* This function does not import instance name because this is more property
 * of a gadget than a function itself */
static int usbg_export_function_prep(usbg_function *f, config_setting_t *root)
{
	config_setting_t *node;
	int ret = USBG_ERROR_NO_MEM;
	int cfg_ret;

	node = config_setting_add(root, USBG_TYPE_TAG, CONFIG_TYPE_STRING);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_string(node, usbg_get_function_type_str(
						    f->type));
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	node = config_setting_add(root, USBG_ATTRS_TAG, CONFIG_TYPE_GROUP);
	if (!node)
		goto out;

	ret = usbg_export_function_attrs(f, node);
	if (ret)
		goto out;

	node = config_setting_add(root, USBG_OS_DESCS_TAG, CONFIG_TYPE_LIST);
	if (!node)
		goto out;

	/* OS Descriptors are optional */
	ret = usbg_export_function_os_descs(f, node);
	if (ret == USBG_ERROR_NOT_SUPPORTED)
		ret = USBG_SUCCESS;
out:
	return ret;
}


static int usbg_export_gadget_functions(usbg_gadget *g, config_setting_t *root)
{
	usbg_function *f;
	config_setting_t *node, *inst_node;
	int ret = USBG_SUCCESS;
	int cfg_ret;
	char label[USBG_MAX_NAME_LENGTH];
	char *func_label;
	int nmb;

	TAILQ_FOREACH(f, &g->functions, fnode) {
		if (f->label) {
			func_label = f->label;
		} else {
			nmb = generate_function_label(f, label, sizeof(label));
			if (nmb >= sizeof(label)) {
				ret = USBG_ERROR_OTHER_ERROR;
				break;
			}
			func_label = label;
		}

		node = config_setting_add(root, func_label, CONFIG_TYPE_GROUP);
		if (!node) {
			ret = USBG_ERROR_NO_MEM;
			break;
		}

		/* Add instance name to identify in this gadget */
		inst_node = config_setting_add(node, USBG_INSTANCE_TAG,
					  CONFIG_TYPE_STRING);
		if (!inst_node) {
			ret = USBG_ERROR_NO_MEM;
			break;
		}

		cfg_ret = config_setting_set_string(inst_node, f->instance);
		if (cfg_ret != CONFIG_TRUE) {
			ret = USBG_ERROR_OTHER_ERROR;
			break;
		}

		ret = usbg_export_function_prep(f, node);
		if (ret != USBG_SUCCESS)
			break;
	}

	return ret;
}

static int usbg_export_gadget_strs_lang(usbg_gadget *g, int lang,
					config_setting_t *root)
{
	config_setting_t *node;
	struct usbg_gadget_strs strs;
	int usbg_ret, cfg_ret;
	int ret = USBG_ERROR_NO_MEM;;

	usbg_ret = usbg_get_gadget_strs(g, lang, &strs);
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, USBG_LANG_TAG, CONFIG_TYPE_INT);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_format(node, CONFIG_FORMAT_HEX);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	cfg_ret = config_setting_set_int(node, lang);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

#define ADD_GADGET_STR(str_name, field)					\
	do {								\
		node = config_setting_add(root, str_name, CONFIG_TYPE_STRING); \
		if (!node)						\
			goto out;					\
		cfg_ret = config_setting_set_string(node, strs.field);	\
		if (cfg_ret != CONFIG_TRUE) {				\
			ret = USBG_ERROR_OTHER_ERROR;			\
			goto out;					\
		}							\
	} while (0)

	ADD_GADGET_STR("manufacturer", manufacturer);
	ADD_GADGET_STR("product", product);
	ADD_GADGET_STR("serialnumber", serial);

#undef ADD_GADGET_STR
	ret = USBG_SUCCESS;
out:
	usbg_free_gadget_strs(&strs);
	return ret;
}

static int usbg_export_gadget_strings(usbg_gadget *g, config_setting_t *root)
{
	config_setting_t *node;
	int *langs;
	int i;
	int usbg_ret = USBG_SUCCESS;

	usbg_ret = usbg_get_gadget_strs_langs(g, &langs);
	if (usbg_ret != USBG_SUCCESS)
		goto out;

	for (i = 0; langs[i]; ++i) {
		node = config_setting_add(root, NULL, CONFIG_TYPE_GROUP);
		if (!node) {
			usbg_ret = USBG_ERROR_NO_MEM;
			break;
		}

		usbg_ret = usbg_export_gadget_strs_lang(g, langs[i], node);
		if (usbg_ret != USBG_SUCCESS)
			break;
	}

	free(langs);
out:
	return usbg_ret;
}

static int usbg_export_gadget_attrs(usbg_gadget *g, config_setting_t *root)
{
	config_setting_t *node;
	struct usbg_gadget_attrs attrs;
	int usbg_ret, cfg_ret;
	int ret = USBG_ERROR_NO_MEM;

	usbg_ret = usbg_get_gadget_attrs(g, &attrs);
	if (usbg_ret) {
		ret = usbg_ret;
		goto out;
	}

#define ADD_GADGET_ATTR(attr_name)					\
	do {								\
		node = config_setting_add(root, #attr_name, CONFIG_TYPE_INT); \
		if (!node)						\
			goto out;					\
		cfg_ret = config_setting_set_format(node, CONFIG_FORMAT_HEX); \
		if (cfg_ret != CONFIG_TRUE) {				\
			ret = USBG_ERROR_OTHER_ERROR;			\
			goto out;					\
		}							\
		cfg_ret = config_setting_set_int(node, attrs.attr_name); \
		if (cfg_ret != CONFIG_TRUE) {				\
			ret = USBG_ERROR_OTHER_ERROR;			\
			goto out;					\
		}							\
	} while (0)

	ADD_GADGET_ATTR(bcdUSB);
	ADD_GADGET_ATTR(bDeviceClass);
	ADD_GADGET_ATTR(bDeviceSubClass);
	ADD_GADGET_ATTR(bDeviceProtocol);
	ADD_GADGET_ATTR(bMaxPacketSize0);
	ADD_GADGET_ATTR(idVendor);
	ADD_GADGET_ATTR(idProduct);
	ADD_GADGET_ATTR(bcdDevice);

#undef ADD_GADGET_ATTR

	ret = 0;
out:
	return ret;
}

static int usbg_export_gadget_os_descs(usbg_gadget *g, config_setting_t *root)
{
	config_setting_t *node;
	struct usbg_gadget_os_descs g_os_descs = {0};
	int usbg_ret, cfg_ret;
	int ret = USBG_ERROR_NO_MEM;

	if (g->os_desc_binding) {
		node = config_setting_add(root, USBG_CONFIG_ID_TAG,
					  CONFIG_TYPE_INT);
		if (!node)
			goto out;

		cfg_ret = config_setting_set_int(node, g->os_desc_binding->id);
		if (cfg_ret != CONFIG_TRUE) {
			ret = USBG_ERROR_OTHER_ERROR;
			goto out;
		}
	}

	usbg_ret = usbg_get_gadget_os_descs(g, &g_os_descs);
	if (usbg_ret) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, "use", CONFIG_TYPE_INT);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_int(node, g_os_descs.use);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	node = config_setting_add(root, "qw_sign", CONFIG_TYPE_STRING);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_string(node, g_os_descs.qw_sign);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	node = config_setting_add(root, "b_vendor_code", CONFIG_TYPE_INT);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_format(node, CONFIG_FORMAT_HEX);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	cfg_ret = config_setting_set_int(node, g_os_descs.b_vendor_code);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	ret = 0;
out:
	usbg_free_gadget_os_desc(&g_os_descs);
	return ret;
}

static int usbg_export_gadget_prep(usbg_gadget *g, config_setting_t *root)
{
	config_setting_t *node;
	int ret = USBG_ERROR_NO_MEM;
	int usbg_ret;

	/* We don't export name tag because name should be given during
	 * loading of gadget */

	node = config_setting_add(root, USBG_ATTRS_TAG, CONFIG_TYPE_GROUP);
	if (!node)
		goto out;

	usbg_ret = usbg_export_gadget_attrs(g, node);
	if (usbg_ret) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, USBG_OS_DESCS_TAG, CONFIG_TYPE_GROUP);
	if (!node)
		goto out;
	usbg_ret = usbg_export_gadget_os_descs(g, node);
	if (usbg_ret && usbg_ret != USBG_ERROR_NOT_FOUND) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, USBG_STRINGS_TAG,
				     CONFIG_TYPE_LIST);
	if (!node)
		goto out;

	usbg_ret = usbg_export_gadget_strings(g, node);
	if (usbg_ret) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, USBG_FUNCTIONS_TAG,
				  CONFIG_TYPE_GROUP);
	if (!node)
		goto out;

	usbg_ret = usbg_export_gadget_functions(g, node);
	if (usbg_ret) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, USBG_CONFIGS_TAG,
				     CONFIG_TYPE_LIST);
	if (!node)
		goto out;

	usbg_ret = usbg_export_gadget_configs(g, node);
	ret = usbg_ret;
out:
	return ret;
}

/* Export gadget/function/config API implementation */

int usbg_export_function(usbg_function *f, FILE *stream)
{
	config_t cfg;
	config_setting_t *root;
	int ret;

	if (!f || !stream)
		return USBG_ERROR_INVALID_PARAM;

	config_init(&cfg);

	/* Set format */
	config_set_tab_width(&cfg, USBG_TAB_WIDTH);

	/* Always successful */
	root = config_root_setting(&cfg);

	ret = usbg_export_function_prep(f, root);
	if (ret != USBG_SUCCESS)
		goto out;

	config_write(&cfg, stream);
out:
	config_destroy(&cfg);
	return ret;
}

int usbg_export_config(usbg_config *c, FILE *stream)
{
	config_t cfg;
	config_setting_t *root;
	int ret;

	if (!c || !stream)
		return USBG_ERROR_INVALID_PARAM;

	config_init(&cfg);

	/* Set format */
	config_set_tab_width(&cfg, USBG_TAB_WIDTH);

	/* Always successful */
	root = config_root_setting(&cfg);

	ret = usbg_export_config_prep(c, root);
	if (ret != USBG_SUCCESS)
		goto out;

	config_write(&cfg, stream);
out:
	config_destroy(&cfg);
	return ret;
}

int usbg_export_gadget(usbg_gadget *g, FILE *stream)
{
	config_t cfg;
	config_setting_t *root;
	int ret;

	if (!g || !stream)
		return USBG_ERROR_INVALID_PARAM;

	config_init(&cfg);

	/* Set format */
	config_set_tab_width(&cfg, USBG_TAB_WIDTH);

	/* Always successful */
	root = config_root_setting(&cfg);

	ret = usbg_export_gadget_prep(g, root);
	if (ret != USBG_SUCCESS)
		goto out;

	config_write(&cfg, stream);
out:
	config_destroy(&cfg);
	return ret;
}

static int split_function_label(const char *label, usbg_function_type *type,
				const char **instance)
{
	const char *floor;
	char buf[USBG_MAX_NAME_LENGTH];
	int len;
	int function_type;
	int ret = USBG_ERROR_NOT_FOUND;

	/* We assume that function type string doesn't contain '_' */
	floor = strchr(label, '_');
	/* if phrase before _ is longer than max name length we may
	 * stop looking */
	len = floor - label;
	if (len >= USBG_MAX_NAME_LENGTH || floor == label)
		goto out;

	strncpy(buf, label, len);
	buf[len] = '\0';

	function_type = usbg_lookup_function_type(buf);
	if (function_type < 0)
		goto out;

	*type = (usbg_function_type)function_type;
	*instance = floor + 1;

	ret = USBG_SUCCESS;
out:
	return ret;
}

static void usbg_set_failed_import(config_t **to_set, config_t *failed)
{
	if (*to_set != NULL) {
		config_destroy(*to_set);
		free(*to_set);
	}

	*to_set = failed;
}

static int usbg_import_function_attrs(config_setting_t *root, usbg_function *f)
{
	int ret = USBG_ERROR_NOT_SUPPORTED;

	if (!f->ops->import)
		goto out;

	ret = f->ops->import(f, root);
out:
	return ret;
}

static int usbg_import_function_os_descs(config_setting_t *root, usbg_function *f)
{
	config_setting_t *node;
	int ret = USBG_SUCCESS;
	struct usbg_function_os_desc f_os_desc = {0};
	const char *iname = f->ops->os_desc_iname;
	int count, i;

	count = config_setting_length(root);

	for (i = 0; i < count; ++i) {
		config_setting_t *interf_node;
		const char *interface;

		node = config_setting_get_elem(root, i);
		if (!node) {
			ret = USBG_ERROR_OTHER_ERROR;
			break;
		}

		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			break;
		}

		/* Look for instance name */
		interf_node = config_setting_get_member(node, USBG_INTERFACE_TAG);
		if (!interf_node) {
			ret = USBG_ERROR_MISSING_TAG;
			break;
		}

		interface = config_setting_get_string(interf_node);
		if (!interface) {
			ret = USBG_ERROR_OTHER_ERROR;
			break;
		}

		if (strcmp(interface, f->ops->os_desc_iname)) {
			ret = USBG_ERROR_NOT_SUPPORTED;
			break;
		}

		ret = usbg_get_config_node_os_desc(node, interface, &f_os_desc);
		if (ret) {
			usbg_free_interf_os_desc(&f_os_desc);
			break;
		}

		ret = usbg_set_interf_os_desc(f, iname, &f_os_desc);
		usbg_free_interf_os_desc(&f_os_desc);
		if (ret)
			break;
	}

out:
	return ret;
}

static int usbg_import_function_run(usbg_gadget *g, config_setting_t *root,
				    const char *instance, usbg_function **f)
{
	config_setting_t *node;
	const char *type_str;
	int usbg_ret;
	int function_type;
	int ret = USBG_ERROR_MISSING_TAG;

	/* function type is mandatory */
	node = config_setting_get_member(root, USBG_TYPE_TAG);
	if (!node)
		goto out;

	type_str = config_setting_get_string(node);
	if (!type_str) {
		ret = USBG_ERROR_INVALID_TYPE;
		goto out;
	}

	/* Check if this type is supported */
	function_type = usbg_lookup_function_type(type_str);
	if (function_type < 0) {
		ret = USBG_ERROR_NOT_SUPPORTED;
		goto out;
	}

	/* All data collected, let's get to work and create this function */
	ret = usbg_create_function(g, (usbg_function_type)function_type,
				   instance, NULL, f);

	if (ret != USBG_SUCCESS)
		goto out;

	/* Attrs are optional */
	node = config_setting_get_member(root, USBG_ATTRS_TAG);
	if (node) {
		usbg_ret = usbg_import_function_attrs(node, *f);
		if (usbg_ret != USBG_SUCCESS) {
			ret = usbg_ret;
			goto out;
		}
	}

	node = config_setting_get_member(root, USBG_OS_DESCS_TAG);
	if (node) {
		usbg_ret = usbg_import_function_os_descs(node, *f);
		if (usbg_ret != USBG_SUCCESS) {
			ret = usbg_ret;
			goto out;
		}
	}

out:
	return ret;
}

static usbg_function *usbg_lookup_function(usbg_gadget *g, const char *label)
{
	usbg_function *f;
	int usbg_ret;

	/* check if such function has also been imported */
	TAILQ_FOREACH(f, &g->functions, fnode) {
		if (f->label && !strcmp(f->label, label))
			break;
	}

	/* if not let's check if label follows the naming convention */
	if (!f) {
		usbg_function_type type;
		const char *instance;

		usbg_ret = split_function_label(label, &type, &instance);
		if (usbg_ret != USBG_SUCCESS)
			goto out;

		/* check if such function exist */
		f = usbg_get_function(g, type, instance);
	}

out:
	return f;
}

/* We have a string which should match with one of function names */
static int usbg_import_binding_string(config_setting_t *root, usbg_config *c)
{
	const char *func_label;
	usbg_function *target;
	int ret;

	func_label = config_setting_get_string(root);
	if (!func_label) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	target = usbg_lookup_function(c->parent, func_label);
	if (!target) {
		ret = USBG_ERROR_NOT_FOUND;
		goto out;
	}

	ret = usbg_add_config_function(c, target->name, target);
out:
	return ret;
}

static int usbg_import_binding_group(config_setting_t *root, usbg_config *c)
{
	config_setting_t *node;
	const char *func_label, *name;
	usbg_function *target;
	int ret;

	node = config_setting_get_member(root, USBG_FUNCTION_TAG);
	if (!node) {
		ret = USBG_ERROR_MISSING_TAG;
		goto out;
	}

	/* It is allowed to provide link to existing function
	 * or define unlabeled instance of function in this place */
	if (usbg_config_is_string(node)) {
		func_label = config_setting_get_string(node);
		if (!func_label) {
			ret = USBG_ERROR_OTHER_ERROR;
			goto out;
		}

		target = usbg_lookup_function(c->parent, func_label);
		if (!target) {
			ret = USBG_ERROR_NOT_FOUND;
			goto out;
		}
	} else if (config_setting_is_group(node)) {
		config_setting_t *inst_node;
		const char *instance;

		inst_node = config_setting_get_member(node, USBG_INSTANCE_TAG);
		if (!inst_node) {
			ret = USBG_ERROR_MISSING_TAG;
			goto out;
		}

		instance = config_setting_get_string(inst_node);
		if (!instance) {
			ret = USBG_ERROR_OTHER_ERROR;
			goto out;
		}

		ret = usbg_import_function_run(c->parent, node,
					       instance, &target);
		if (ret != USBG_SUCCESS)
			goto out;
	} else {
		ret = USBG_ERROR_INVALID_TYPE;
		goto out;
	}

	/* Name tag is optional. When no such tag, default one will be used */
	node = config_setting_get_member(root, USBG_NAME_TAG);
	if (node) {
		if (!usbg_config_is_string(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto out;
		}

		name = config_setting_get_string(node);
		if (!name) {
			ret = USBG_ERROR_OTHER_ERROR;
			goto out;
		}
	} else {
		name = target->name;
	}

	ret = usbg_add_config_function(c, name, target);
out:
	return ret;
}

static int usbg_import_config_bindings(config_setting_t *root, usbg_config *c)
{
	config_setting_t *node;
	int ret = USBG_SUCCESS;
	int count, i;

	count = config_setting_length(root);

	for (i = 0; i < count; ++i) {
		node = config_setting_get_elem(root, i);

		if (usbg_config_is_string(node))
			ret = usbg_import_binding_string(node, c);
		else if (config_setting_is_group(node))
			ret = usbg_import_binding_group(node, c);
		else
			ret = USBG_ERROR_INVALID_TYPE;

		if (ret != USBG_SUCCESS)
			break;
	}

	return ret;
}

static int usbg_import_config_strs_lang(config_setting_t *root, usbg_config *c)
{
	config_setting_t *node;
	int lang;
	struct usbg_config_strs c_strs = {0};
	int ret = USBG_ERROR_INVALID_TYPE;

	node = config_setting_get_member(root, USBG_LANG_TAG);
	if (!node) {
		ret = USBG_ERROR_MISSING_TAG;
		goto out;
	}

	if (!usbg_config_is_int(node))
		goto out;

	lang = config_setting_get_int(node);

	/* Configuration string is optional */
	node = config_setting_get_member(root, "configuration");
	if (node) {
		if (!usbg_config_is_string(node))
			goto out;

		c_strs.configuration = (char *)config_setting_get_string(node);
	}

	ret = usbg_set_config_strs(c, lang, &c_strs);

out:
	return ret;
}

static int usbg_import_config_strings(config_setting_t *root, usbg_config *c)
{
	config_setting_t *node;
	int ret = USBG_SUCCESS;
	int count, i;

	count = config_setting_length(root);

	for (i = 0; i < count; ++i) {
		node = config_setting_get_elem(root, i);
		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			break;
		}

		ret = usbg_import_config_strs_lang(node, c);
		if (ret != USBG_SUCCESS)
			break;
	}

	return ret;
}

static int usbg_import_config_attrs(config_setting_t *root, usbg_config *c)
{
	config_setting_t *node;
	int usbg_ret;
	int bmAttributes, bMaxPower;
	int ret = USBG_ERROR_INVALID_TYPE;

	node = config_setting_get_member(root, "bmAttributes");
	if (node) {
		if (!usbg_config_is_int(node))
			goto out;

		bmAttributes = config_setting_get_int(node);
		usbg_ret = usbg_set_config_bm_attrs(c, bmAttributes);
		if (usbg_ret != USBG_SUCCESS) {
			ret = usbg_ret;
			goto out;
		}
	}

	node = config_setting_get_member(root, "bMaxPower");
	if (node) {
		if (!usbg_config_is_int(node))
			goto out;

		bMaxPower = config_setting_get_int(node);
		usbg_ret = usbg_set_config_max_power(c, bMaxPower);
		if (usbg_ret != USBG_SUCCESS) {
			ret = usbg_ret;
			goto out;
		}
	}

	/* Empty attrs section is also considered to be valid */
	ret = USBG_SUCCESS;
out:
	return ret;

}

static int usbg_import_config_run(usbg_gadget *g, config_setting_t *root,
				  int id, usbg_config **c)
{
	config_setting_t *node;
	const char *name;
	usbg_config *newc;
	int usbg_ret;
	int ret = USBG_ERROR_MISSING_TAG;

	/*
	 * Label is mandatory,
	 * if attrs aren't present defaults are used
	 */
	node = config_setting_get_member(root, USBG_NAME_TAG);
	if (!node)
		goto out;

	name = config_setting_get_string(node);
	if (!name) {
		ret = USBG_ERROR_INVALID_TYPE;
		goto out;
	}

	/* Required data collected, let's create our config */
	usbg_ret = usbg_create_config(g, id, name, NULL, NULL, &newc);
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

	/* Attrs are optional */
	node = config_setting_get_member(root, USBG_ATTRS_TAG);
	if (node) {
		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto error2;
		}

		usbg_ret = usbg_import_config_attrs(node, newc);
		if (usbg_ret != USBG_SUCCESS)
			goto error;
	}

	/* Strings are also optional */
	node = config_setting_get_member(root, USBG_STRINGS_TAG);
	if (node) {
		if (!config_setting_is_list(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto error2;
		}

		usbg_ret = usbg_import_config_strings(node, newc);
		if (usbg_ret != USBG_SUCCESS)
			goto error;
	}

	/* Functions too, because some config may not be
	 * fully configured and not contain any function */
	node = config_setting_get_member(root, USBG_FUNCTIONS_TAG);
	if (node) {
		if (!config_setting_is_list(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto error2;
		}

		usbg_ret = usbg_import_config_bindings(node, newc);
		if (usbg_ret != USBG_SUCCESS)
			goto error;
	}

	*c = newc;
	ret = USBG_SUCCESS;
out:
	return ret;

error:
	ret = usbg_ret;
error2:
	/* We ignore returned value, if function fails
	 * there is no way to handle it */
	usbg_rm_config(newc, USBG_RM_RECURSE);
	return ret;
}

static int usbg_import_gadget_configs(config_setting_t *root, usbg_gadget *g)
{
	config_setting_t *node, *id_node;
	int id;
	usbg_config *c;
	int ret = USBG_SUCCESS;
	int count, i;

	count = config_setting_length(root);

	for (i = 0; i < count; ++i) {
		node = config_setting_get_elem(root, i);
		if (!node) {
			ret = USBG_ERROR_OTHER_ERROR;
			break;
		}

		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			break;
		}

		/* Look for id */
		id_node = config_setting_get_member(node, USBG_ID_TAG);
		if (!id_node) {
			ret = USBG_ERROR_MISSING_TAG;
			break;
		}

		if (!usbg_config_is_int(id_node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			break;
		}

		id = config_setting_get_int(id_node);

		ret = usbg_import_config_run(g, node, id, &c);
		if (ret != USBG_SUCCESS)
			break;
	}

	return ret;
}

static int usbg_import_gadget_functions(config_setting_t *root, usbg_gadget *g)
{
	config_setting_t *node, *inst_node;
	const char *instance;
	const char *label;
	usbg_function *f;
	int ret = USBG_SUCCESS;
	int count, i;

	count = config_setting_length(root);

	for (i = 0; i < count; ++i) {
		node = config_setting_get_elem(root, i);
		if (!node) {
			ret = USBG_ERROR_OTHER_ERROR;
			break;
		}

		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			break;
		}

		/* Look for instance name */
		inst_node = config_setting_get_member(node, USBG_INSTANCE_TAG);
		if (!inst_node) {
			ret = USBG_ERROR_MISSING_TAG;
			break;
		}

		if (!usbg_config_is_string(inst_node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			break;
		}

		instance = config_setting_get_string(inst_node);
		if (!instance) {
			ret = USBG_ERROR_OTHER_ERROR;
			break;
		}

		ret = usbg_import_function_run(g, node, instance, &f);
		if (ret != USBG_SUCCESS)
			break;

		/* Set the label given by user */
		label = config_setting_name(node);
		if (!label) {
			ret = USBG_ERROR_OTHER_ERROR;
			break;
		}

		f->label = strdup(label);
		if (!f->label) {
			ret = USBG_ERROR_NO_MEM;
			break;
		}
	}

	return ret;
}

static int usbg_import_gadget_strs_lang(config_setting_t *root, usbg_gadget *g)
{
	config_setting_t *node;
	int lang;
	struct usbg_gadget_strs g_strs = {0};
	int ret = USBG_ERROR_INVALID_TYPE;

	node = config_setting_get_member(root, USBG_LANG_TAG);
	if (!node) {
		ret = USBG_ERROR_MISSING_TAG;
		goto out;
	}

	if (!usbg_config_is_int(node))
		goto out;

	lang = config_setting_get_int(node);

	/* Auto truncate the string to max length */
#define GET_OPTIONAL_GADGET_STR(NAME, FIELD)				\
	do {								\
		node = config_setting_get_member(root, #NAME);		\
		if (node) {						\
			if (!usbg_config_is_string(node))		\
				goto out;				\
			g_strs.FIELD = (char *)config_setting_get_string(node); \
		}							\
	} while (0)

	GET_OPTIONAL_GADGET_STR(manufacturer, manufacturer);
	GET_OPTIONAL_GADGET_STR(product, product);
	GET_OPTIONAL_GADGET_STR(serialnumber, serial);

#undef GET_OPTIONAL_GADGET_STR

	ret = usbg_set_gadget_strs(g, lang, &g_strs);

out:
	return ret;
}

static int usbg_import_gadget_strings(config_setting_t *root, usbg_gadget *g)
{
	config_setting_t *node;
	int ret = USBG_SUCCESS;
	int count, i;

	count = config_setting_length(root);

	for (i = 0; i < count; ++i) {
		node = config_setting_get_elem(root, i);
		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			break;
		}

		ret = usbg_import_gadget_strs_lang(node, g);
		if (ret != USBG_SUCCESS)
			break;
	}

	return ret;
}


static int usbg_import_gadget_attrs(config_setting_t *root, usbg_gadget *g)
{
	config_setting_t *node;
	int usbg_ret;
	int val;
	int ret = USBG_ERROR_INVALID_TYPE;

#define GET_OPTIONAL_GADGET_ATTR(NAME, FUNC_END, TYPE)			\
	do {								\
		node = config_setting_get_member(root, #NAME);		\
		if (node) {						\
			if (!usbg_config_is_int(node))			\
				goto out;				\
			val = config_setting_get_int(node);		\
			if (val < 0 || val > ((1L << (sizeof(TYPE)*8)) - 1)) { \
				ret = USBG_ERROR_INVALID_VALUE;		\
				goto out;				\
			}						\
			usbg_ret = usbg_set_gadget_##FUNC_END(g, (TYPE)val); \
			if (usbg_ret != USBG_SUCCESS) {			\
				ret = usbg_ret;				\
				goto out;				\
			}						\
		}							\
	} while (0)

	GET_OPTIONAL_GADGET_ATTR(bcdUSB, device_bcd_usb, uint16_t);
	GET_OPTIONAL_GADGET_ATTR(bDeviceClass, device_class, uint8_t);
	GET_OPTIONAL_GADGET_ATTR(bDeviceSubClass, device_subclass, uint8_t);
	GET_OPTIONAL_GADGET_ATTR(bDeviceProtocol, device_protocol, uint8_t);
	GET_OPTIONAL_GADGET_ATTR(bMaxPacketSize0, device_max_packet, uint8_t);
	GET_OPTIONAL_GADGET_ATTR(idVendor, vendor_id, uint16_t);
	GET_OPTIONAL_GADGET_ATTR(idProduct, product_id, uint16_t);
	GET_OPTIONAL_GADGET_ATTR(bcdDevice, device_bcd_device, uint16_t);

#undef GET_OPTIONAL_GADGET_ATTR

	/* Empty attrs section is also considered to be valid */
	ret = USBG_SUCCESS;
out:
	return ret;

}

static int usbg_import_gadget_os_descs(config_setting_t *root, usbg_gadget *g)
{
	config_setting_t *node;
	int usbg_ret;
	int val;
	int ret = USBG_ERROR_INVALID_TYPE;
	const char *str;
	struct usbg_gadget_os_descs g_os_descs = {0};

#define GET_OPTIONAL_GADGET_ATTR(NAME, FIELD, TYPE)			\
	do {								\
		node = config_setting_get_member(root, #NAME);		\
		if (node) {						\
			if (!usbg_config_is_int(node))			\
				goto out;				\
			val = config_setting_get_int(node);		\
			if (val < 0 || val > ((1L << (sizeof(TYPE)*8)) - 1)) { \
				ret = USBG_ERROR_INVALID_VALUE;		\
				goto out;				\
			}						\
			g_os_descs.FIELD = (TYPE)val;			\
		}							\
	} while (0)

	GET_OPTIONAL_GADGET_ATTR(use, use, bool);
	GET_OPTIONAL_GADGET_ATTR(b_vendor_code, b_vendor_code, uint8_t);

#undef GET_OPTIONAL_GADGET_ATTR

	node = config_setting_get_member(root, "qw_sign");
	if (node) {
		if (!usbg_config_is_string(node))
			goto out;
		str = config_setting_get_string(node);
		g_os_descs.qw_sign = strdup(str);
	}

	ret = usbg_set_gadget_os_descs(g, &g_os_descs);

	/*
	 * Configs are optional, because some config may not be
	 * fully configured and not contain any config yet
	 */
	node = config_setting_get_member(root, USBG_CONFIG_ID_TAG);
	if (node) {
		usbg_config *target;
		if (!usbg_config_is_int(node))
			goto out;

		val = config_setting_get_int(node);
		target = usbg_get_config(g, val, NULL);
		if (!target) {
			ret = USBG_ERROR_NOT_FOUND;
			goto out;
		}

		usbg_ret = usbg_set_os_desc_config(g, target);
		if (usbg_ret != USBG_SUCCESS)
			goto out;
	}

out:
	usbg_free_gadget_os_desc(&g_os_descs);
	return ret;
}

static int usbg_import_gadget_run(usbg_state *s, config_setting_t *root,
				  const char *name, usbg_gadget **g)
{
	config_setting_t *node;
	usbg_gadget *newg;
	int usbg_ret;
	int ret = USBG_ERROR_MISSING_TAG;

	/* There is no mandatory data in gadget so let's start with
	 * creating a new gadget */
	usbg_ret = usbg_create_gadget(s, name, NULL, NULL, &newg);
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

	/* Attrs are optional */
	node = config_setting_get_member(root, USBG_ATTRS_TAG);
	if (node) {
		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto error2;
		}

		usbg_ret = usbg_import_gadget_attrs(node, newg);
		if (usbg_ret != USBG_SUCCESS)
			goto error;
	}

	/* Strings are also optional */
	node = config_setting_get_member(root, USBG_STRINGS_TAG);
	if (node) {
		if (!config_setting_is_list(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto error2;
		}

		usbg_ret = usbg_import_gadget_strings(node, newg);
		if (usbg_ret != USBG_SUCCESS)
			goto error;
	}

	/* Functions too, because some gadgets may not be fully
	* configured and don't have any function or have all functions
	* defined inline in configurations */
	node = config_setting_get_member(root, USBG_FUNCTIONS_TAG);
	if (node) {
		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto error2;
		}
		usbg_ret = usbg_import_gadget_functions(node, newg);
		if (usbg_ret != USBG_SUCCESS)
			goto error;
	}

	/* Some gadget may not be fully configured
	 * so configs are also optional */
	node = config_setting_get_member(root, USBG_CONFIGS_TAG);
	if (node) {
		if (!config_setting_is_list(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto error2;
		}
		usbg_ret = usbg_import_gadget_configs(node, newg);
		if (usbg_ret != USBG_SUCCESS)
			goto error;
	}

	/* OS Descriptors are optional too, read after configs */
	node = config_setting_get_member(root, USBG_OS_DESCS_TAG);
	if (node) {
		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto error2;
		}

		usbg_ret = usbg_import_gadget_os_descs(node, newg);
		if (usbg_ret != USBG_SUCCESS)
			goto error;
	}

	*g = newg;
	ret = USBG_SUCCESS;
out:
	return ret;

error:
	ret = usbg_ret;
error2:
	/* We ignore returned value, if function fails
	 * there is no way to handle it */
	usbg_rm_gadget(newg, USBG_RM_RECURSE);
	return ret;
}

int usbg_import_function(usbg_gadget *g, FILE *stream, const char *instance,
			 usbg_function **f)
{
	config_t *cfg;
	config_setting_t *root;
	usbg_function *newf;
	int ret, cfg_ret;

	if (!g || !stream || !instance)
		return USBG_ERROR_INVALID_PARAM;

	cfg = malloc(sizeof(*cfg));
	if (!cfg)
		return USBG_ERROR_NO_MEM;

	config_init(cfg);

	cfg_ret = config_read(cfg, stream);
	if (cfg_ret != CONFIG_TRUE) {
		usbg_set_failed_import(&g->last_failed_import, cfg);
		ret = USBG_ERROR_INVALID_FORMAT;
		goto out;
	}

	/* Always successful */
	root = config_root_setting(cfg);

	ret = usbg_import_function_run(g, root, instance, &newf);
	if (ret != USBG_SUCCESS) {
		usbg_set_failed_import(&g->last_failed_import, cfg);
		goto out;
	}

	if (f)
		*f = newf;

	config_destroy(cfg);
	free(cfg);
	/* Clean last error */
	usbg_set_failed_import(&g->last_failed_import, NULL);
out:
	return ret;

}

int usbg_import_config(usbg_gadget *g, FILE *stream, int id,  usbg_config **c)
{
	config_t *cfg;
	config_setting_t *root;
	usbg_config *newc;
	int ret, cfg_ret;

	if (!g || !stream || id < 0)
		return USBG_ERROR_INVALID_PARAM;

	cfg = malloc(sizeof(*cfg));
	if (!cfg)
		return USBG_ERROR_NO_MEM;

	config_init(cfg);

	cfg_ret = config_read(cfg, stream);
	if (cfg_ret != CONFIG_TRUE) {
		usbg_set_failed_import(&g->last_failed_import, cfg);
		ret = USBG_ERROR_INVALID_FORMAT;
		goto out;
	}

	/* Always successful */
	root = config_root_setting(cfg);

	ret = usbg_import_config_run(g, root, id, &newc);
	if (ret != USBG_SUCCESS) {
		usbg_set_failed_import(&g->last_failed_import, cfg);
		goto out;
	}

	if (c)
		*c = newc;

	config_destroy(cfg);
	free(cfg);
	/* Clean last error */
	usbg_set_failed_import(&g->last_failed_import, NULL);
out:
	return ret;
}

int usbg_import_gadget(usbg_state *s, FILE *stream, const char *name,
		       usbg_gadget **g)
{
	config_t *cfg;
	config_setting_t *root;
	usbg_gadget *newg;
	int ret, cfg_ret;

	if (!s || !stream || !name)
		return USBG_ERROR_INVALID_PARAM;

	cfg = malloc(sizeof(*cfg));
	if (!cfg)
		return USBG_ERROR_NO_MEM;

	config_init(cfg);

	cfg_ret = config_read(cfg, stream);
	if (cfg_ret != CONFIG_TRUE) {
		usbg_set_failed_import(&s->last_failed_import, cfg);
		ret = USBG_ERROR_INVALID_FORMAT;
		goto out;
	}

	/* Always successful */
	root = config_root_setting(cfg);

	ret = usbg_import_gadget_run(s, root, name, &newg);
	if (ret != USBG_SUCCESS) {
		usbg_set_failed_import(&s->last_failed_import, cfg);
		goto out;
	}

	if (g)
		*g = newg;

	config_destroy(cfg);
	free(cfg);
	/* Clean last error */
	usbg_set_failed_import(&s->last_failed_import, NULL);
out:
	return ret;
}

const char *usbg_get_func_import_error_text(usbg_gadget *g)
{
	if (!g || !g->last_failed_import)
		return NULL;

	return config_error_text(g->last_failed_import);
}

int usbg_get_func_import_error_line(usbg_gadget *g)
{
	if (!g || !g->last_failed_import)
		return -1;

	return config_error_line(g->last_failed_import);
}

const char *usbg_get_config_import_error_text(usbg_gadget *g)
{
	if (!g || !g->last_failed_import)
		return NULL;

	return config_error_text(g->last_failed_import);
}

int usbg_get_config_import_error_line(usbg_gadget *g)
{
	if (!g || !g->last_failed_import)
		return -1;

	return config_error_line(g->last_failed_import);
}

const char *usbg_get_gadget_import_error_text(usbg_state *s)
{
	if (!s || !s->last_failed_import)
		return NULL;

	return config_error_text(s->last_failed_import);
}

int usbg_get_gadget_import_error_line(usbg_state *s)
{
	if (!s || !s->last_failed_import)
		return -1;

	return config_error_line(s->last_failed_import);
}
