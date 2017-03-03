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

#ifdef __cplusplus
extern "C" {
#endif

#define usbg_get_config_node_int	NULL
#define usbg_get_config_node_bool	NULL
#define usbg_get_config_node_string	NULL
#define usbg_get_config_node_ether_addr NULL
#define usbg_set_config_node_int	NULL
#define usbg_set_config_node_int_hex	NULL
#define usbg_set_config_node_bool	NULL
#define usbg_set_config_node_string	NULL
#define usbg_set_config_node_ether_addr NULL
#define usbg_set_config_node_dev	NULL

typedef struct _should_not_be_used config_t;
typedef struct _should_not_be_used config_setting_t;
void config_destroy(config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* USBG_INTERNAL_LIBCONFIG_H */
