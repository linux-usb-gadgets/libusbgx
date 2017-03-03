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
#ifndef USBG_INTERNAL_LIBCONFIG_H
#define USBG_INTERNAL_LIBCONFIG_H

#include <libconfig.h>
#ifdef __cplusplus
extern "C" {
#endif

int usbg_get_config_node_int(config_setting_t *root,
			     const char *node_name, void *val);

int usbg_get_config_node_bool(config_setting_t *root,
			      const char *node_name, void *val);

int usbg_get_config_node_string(config_setting_t *root,
				const char *node_name, void *val);

int usbg_get_config_node_ether_addr(config_setting_t *root,
				    const char *node_name, void *val);

int usbg_set_config_node_int(config_setting_t *root,
			     const char *node_name, void *val);

int usbg_set_config_node_int_hex(config_setting_t *root,
				 const char *node_name, void *val);

int usbg_set_config_node_bool(config_setting_t *root,
			      const char *node_name, void *val);

int usbg_set_config_node_string(config_setting_t *root,
				const char *node_name, void *val);

int usbg_set_config_node_ether_addr(config_setting_t *root,
				    const char *node_name, void *val);

int usbg_set_config_node_dev(config_setting_t *root,
			     const char *node_name, void *val);

#ifdef __cplusplus
}
#endif

#endif /* USBG_INTERNAL_LIBCONFIG_H */
