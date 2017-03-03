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
#include "usbg/usbg_internal_libconfig.h"

int usbg_get_config_node_int(config_setting_t *root,
			     const char *node_name, void *val)
{
	config_setting_t *node;

	node = config_setting_get_member(root, node_name);
	if (!node)
		return 0;

	if (!usbg_config_is_int(node))
		return USBG_ERROR_INVALID_TYPE;

	*(int *)val = config_setting_get_int(node);

	return 1;
}

int usbg_get_config_node_bool(config_setting_t *root,
			      const char *node_name, void *val)
{
	config_setting_t *node;
	bool *value = val;
	int ret;

	node = config_setting_get_member(root, node_name);
	if (!node)
		return 0;

	ret = config_setting_type(node);
	switch (ret) {
	case CONFIG_TYPE_INT:
		*value = !!config_setting_get_int(node);
		break;
	case CONFIG_TYPE_BOOL:
		*value = config_setting_get_bool(node);
		break;
	default:
		return USBG_ERROR_INVALID_TYPE;
	}

	return 0;
}

int usbg_get_config_node_string(config_setting_t *root,
				const char *node_name, void *val)
{
	config_setting_t *node;

	node = config_setting_get_member(root, node_name);
	if (!node)
		return 0;

	if (!usbg_config_is_string(node))
		return USBG_ERROR_INVALID_TYPE;

	*(const char **)val = config_setting_get_string(node);

	return 1;
}

int usbg_get_config_node_ether_addr(config_setting_t *root,
				    const char *node_name, void *val)
{
	const char *str_addr;
	struct ether_addr *addr;
	int ret;

	ret = usbg_get_config_node_string(root, node_name, &str_addr);
	/* if not found or error */
	if (ret == 0 || ret < 0)
		return ret;

	addr = ether_aton_r(str_addr, val);

	return addr ? 1 : USBG_ERROR_INVALID_VALUE;
}

int usbg_set_config_node_int(config_setting_t *root,
			     const char *node_name, void *val)
{
	config_setting_t *node;
	int ret = 0;

	node = config_setting_add(root, node_name, CONFIG_TYPE_INT);
	if (!node)
		return USBG_ERROR_NO_MEM;

	ret = config_setting_set_int(node, *(int *)val);

	return ret == CONFIG_TRUE ? 0 : USBG_ERROR_OTHER_ERROR;
}

int usbg_set_config_node_int_hex(config_setting_t *root,
				 const char *node_name, void *val)
{
	config_setting_t *node;
	int ret = 0;

	node = config_setting_add(root, node_name, CONFIG_TYPE_INT);
	if (!node)
		return USBG_ERROR_NO_MEM;

	ret = config_setting_set_format(node, CONFIG_FORMAT_HEX);
	if (ret != CONFIG_TRUE)
		return USBG_ERROR_OTHER_ERROR;

	ret = config_setting_set_int(node, *(int *)val);

	return ret == CONFIG_TRUE ? 0 : USBG_ERROR_OTHER_ERROR;
}

int usbg_set_config_node_bool(config_setting_t *root,
			      const char *node_name, void *val)
{
	config_setting_t *node;
	int ret = 0;

	node = config_setting_add(root, node_name, CONFIG_TYPE_BOOL);
	if (!node)
		return USBG_ERROR_NO_MEM;

	ret = config_setting_set_bool(node, *(bool *)val);

	return ret == CONFIG_TRUE ? 0 : USBG_ERROR_OTHER_ERROR;
}

int usbg_set_config_node_string(config_setting_t *root,
				const char *node_name, void *val)
{
	config_setting_t *node;
	int ret = 0;

	node = config_setting_add(root, node_name, CONFIG_TYPE_STRING);
	if (!node)
		return USBG_ERROR_NO_MEM;

	ret = config_setting_set_string(node, *(char **)val);

	return ret == CONFIG_TRUE ? 0 : USBG_ERROR_OTHER_ERROR;
}

int usbg_set_config_node_ether_addr(config_setting_t *root,
				    const char *node_name, void *val)
{
	char str_addr[USBG_MAX_STR_LENGTH];
	char *ptr = str_addr;

	usbg_ether_ntoa_r(val, str_addr);
	return usbg_set_config_node_string(root, node_name, &ptr);
}

int usbg_set_config_node_dev(config_setting_t *root,
			     const char *node_name, void *val)
{
	dev_t *dev = (dev_t *)val;
	config_setting_t *node;
	int tmp;
	int ret = 0;

	node = config_setting_add(root, node_name, CONFIG_TYPE_GROUP);
	if (!node)
		return USBG_ERROR_NO_MEM;

	tmp = major(*dev);
	ret = usbg_set_config_node_int(node, "major", &tmp);
	if (ret)
		return ret;

	tmp = minor(*dev);
	ret = usbg_set_config_node_int(node, "minor", &tmp);

	return ret;
}

