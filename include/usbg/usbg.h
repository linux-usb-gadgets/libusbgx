/*
 * Copyright (C) 2013 Linaro Limited
 *
 * Matt Porter <mporter@linaro.org>
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

#ifndef __USBG_H__
#define __USBG_H__

#include <dirent.h>
#include <sys/queue.h>
#include <netinet/ether.h>
#include <stdint.h>

/**
 * @file include/usbg/usbg.h
 * @todo Add usbg_remove_[gadget|config|function|binding] APIs
 * @todo Clean up static buffers in structures
 */

/**
 * @addtogroup libusbgx
 * Public API for USB gadget-configfs library
 * @{
 */

#define DEFAULT_UDC		NULL
#define LANG_US_ENG		0x0409

#define USBG_MAX_STR_LENGTH 256
#define USBG_MAX_PATH_LENGTH 256
#define USBG_MAX_NAME_LENGTH 40

/**
 * @struct state
 * @brief State of the gadget devices in the system
 */
struct state
{
	char path[USBG_MAX_PATH_LENGTH];

	TAILQ_HEAD(ghead, gadget) gadgets;
};

/**
 * @struct gadget_attrs
 * @brief USB gadget device attributes
 */
struct gadget_attrs
{
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
};

/**
 * @struct gadget_strs
 * @brief USB gadget device strings
 */
struct gadget_strs
{
	char str_ser[USBG_MAX_STR_LENGTH];
	char str_mnf[USBG_MAX_STR_LENGTH];
	char str_prd[USBG_MAX_STR_LENGTH];
};

/**
 * @struct gadget
 * @brief USB gadget device
 */
struct gadget
{
	char name[USBG_MAX_NAME_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];
	char udc[USBG_MAX_STR_LENGTH];

	struct gadget_attrs attrs;
	struct gadget_strs strs;

	TAILQ_ENTRY(gadget) gnode;
	TAILQ_HEAD(chead, config) configs;
	TAILQ_HEAD(fhead, function) functions;
	struct state *parent;
};

/**
 * @struct config_attrs
 * @brief USB configuration attributes
 */
struct config_attrs
{
	uint8_t bmAttributes;
	uint8_t bMaxPower;
};

/**
 * @struct config_strs
 * @brief USB configuration strings
 */
struct config_strs
{
	char configuration[USBG_MAX_STR_LENGTH];
};

/**
 * @struct config
 * @brief USB gadget configuration attributes
 */
struct config
{
	TAILQ_ENTRY(config) cnode;
	TAILQ_HEAD(bhead, binding) bindings;
	struct gadget *parent;

	char name[USBG_MAX_NAME_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];
	struct config_attrs attrs;
	struct config_strs strs;
};

/**
 * @enum function_type
 * @brief Supported USB function types
 */
enum function_type
{
	F_SERIAL,
	F_ACM,
	F_OBEX,
	F_ECM,
	F_SUBSET,
	F_NCM,
	F_EEM,
	F_RNDIS,
	F_PHONET,
};

/**
 * @var function_names
 * @brief Name strings for supported USB function types
 */
const char *function_names[] =
{
	"gser",
	"acm",
	"obex",
	"ecm",
	"geth",
	"ncm",
	"eem",
	"rndis",
	"phonet",
};

/**
 * @struct serial_attrs
 * @brief Attributes for Serial, ACM, and OBEX USB functions
 */
struct serial_attrs {
	int port_num;
};

/**
 * @struct net_attrs
 * @brief Attributes for ECM, ECM subset, NCM, EEM, and RNDIS USB functions
 */
struct net_attrs {
	struct ether_addr dev_addr;
	struct ether_addr host_addr;
	char ifname[USBG_MAX_STR_LENGTH];
	int qmult;
};

/**
 * @struct phonet_attrs
 * @brief Attributes for the phonet USB function
 */
struct phonet_attrs {
	char ifname[USBG_MAX_STR_LENGTH];
};

/**
 * @union attrs
 * @brief Attributes for a given function type
 */
union attrs {
	struct serial_attrs serial;
	struct net_attrs net;
	struct phonet_attrs phonet;
};

/**
 * @struct function
 * @brief USB gadget function attributes
 */
struct function
{
	TAILQ_ENTRY(function) fnode;
	struct gadget *parent;

	char name[USBG_MAX_NAME_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];

	enum function_type type;
	union attrs attr;
};


/**
 * @struct binding
 * @brief Describes a binding between a USB gadget configuration
 *	  and a USB gadget function
 */
struct binding
{
	TAILQ_ENTRY(binding) bnode;
	struct config *parent;
	struct function *target;

	char name[USBG_MAX_NAME_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];
};

/* Library init and cleanup */

/**
 * @brief Initialize the libusbgx library state
 * @param configfs_path Path to the mounted configfs filesystem
 * @return Pointer to a state structure
 */
extern struct state *usbg_init(char *configfs_path);

/**
 * @brief Clean up the libusbgx library state
 * @param s Pointer to state
 */
extern void usbg_cleanup(struct state *s);

/**
 * @brief Get ConfigFS path length
 * @param s Pointer to state
 * @return Length of path or -1 if error occurred.
 */
extern size_t usbg_get_configfs_path_len(struct state *s);

/**
 * @brieg Get ConfigFS path
 * @param s Pointer to state
 * @param buf Buffer where path should be copied
 * @param len Length of given buffer
 * @return Pointer to destination or NULL if error occurred.
 */
extern char *usbg_get_configfs_path(struct state *s, char *buf, size_t len);

/* USB gadget queries */

/**
 * @brief Get a gadget device by name
 * @param s Pointer to state
 * @param name Name of the gadget device
 * @return Pointer to gadget or NULL if a matching gadget isn't found
 */
extern struct gadget *usbg_get_gadget(struct state *s, const char *name);

/**
 * @brief Get a function by name
 * @param g Pointer to gadget
 * @param name Name of the function
 * @return Pointer to function or NULL if a matching function isn't found
 */
extern struct function *usbg_get_function(struct gadget *g, const char *name);

/**
 * @brief Get a configuration by name
 * @param g Pointer to gadget
 * @param name Name of the configuration
 * @return Pointer to config or NULL if a matching config isn't found
 */
extern struct config *usbg_get_config(struct gadget *g, const char *name);

/* USB gadget allocation and configuration */

/**
 * @brief Create a new USB gadget device
 * @param s Pointer to state
 * @param name Name of the gadget
 * @param idVendor Gadget vendor ID
 * @param idProduct Gadget product ID
 * @return Pointer to gadget or NULL if the gadget cannot be created
 */
extern struct gadget *usbg_create_gadget_vid_pid(struct state *s, char *name,
		uint16_t idVendor, uint16_t idProduct);

/**
 * @brief Create a new USB gadget device and set given attributes
 * and strings
 * @param s Pointer to state
 * @param name Name of the gadget
 * @param g_attrs Gadget attributes to be set. If NULL setting is omitted.
 * @param g_strs Gadget strings to be set. If NULL setting is omitted.
 * @note Given strings are assumed to be in US English
 * @return Pointer to gadget or NULL if the gadget cannot be created
 */
extern struct gadget *usbg_create_gadget(struct state *s, char *name,
		struct gadget_attrs *g_attrs, struct gadget_strs *g_strs);

/**
 * @brief Set the USB gadget attributes
 * @param g Pointer to gadget
 * @param g_attrs Gadget attributes
 */
extern void usbg_set_gadget_attrs(struct gadget *g,
		struct gadget_attrs *g_attrs);

/**
 * @brief Get the USB gadget strings
 * @param g Pointer to gadget
 * @param g_attrs Structure to be filled
 * @retur Pointer to filled structure or NULL if error occurred.
 */
extern struct gadget_attrs *usbg_get_gadget_attrs(struct gadget *g,
		struct gadget_attrs *g_attrs);

/**
 * @brief Get gadget name length
 * @param g Gadget which name length should be returned
 * @return Length of name string or -1 if error occurred.
 */
extern size_t usbg_get_gadget_name_len(struct gadget *g);

/**
 * @brieg Get gadget name
 * @param b Pointer to gadget
 * @param buf Buffer where name should be copied
 * @param len Length of given buffer
 * @return Pointer to destination or NULL if error occurred.
 */
extern char *usbg_get_gadget_name(struct gadget *g, char *buf, size_t len);

/**
 * @brief Set the USB gadget vendor id
 * @param g Pointer to gadget
 * @param idVendor USB device vendor id
 */
extern void usbg_set_gadget_vendor_id(struct gadget *g, uint16_t idVendor);

/**
 * @brief Set the USB gadget product id
 * @param g Pointer to gadget
 * @param idProduct USB device product id
 */
extern void usbg_set_gadget_product_id(struct gadget *g, uint16_t idProduct);

/**
 * @brief Set the USB gadget device class code
 * @param g Pointer to gadget
 * @param bDeviceClass USB device class code
 */
extern void usbg_set_gadget_device_class(struct gadget *g,
		uint8_t bDeviceClass);

/**
 * @brief Set the USB gadget protocol code
 * @param g Pointer to gadget
 * @param bDeviceProtocol USB protocol code
 */
extern void usbg_set_gadget_device_protocol(struct gadget *g,
		uint8_t bDeviceProtocol);

/**
 * @brief Set the USB gadget device subclass code
 * @param g Pointer to gadget
 * @param bDeviceSubClass USB device subclass code
 */
extern void usbg_set_gadget_device_subclass(struct gadget *g,
		uint8_t bDeviceSubClass);

/**
 * @brief Set the maximum packet size for a gadget
 * @param g Pointer to gadget
 * @param bMaxPacketSize0 Maximum packet size
 */
extern void usbg_set_gadget_device_max_packet(struct gadget *g,
		uint8_t bMaxPacketSize0);

/**
 * @brief Set the gadget device BCD release number
 * @param g Pointer to gadget
 * @param bcdDevice BCD release number
 */
extern void usbg_set_gadget_device_bcd_device(struct gadget *g,
		uint16_t bcdDevice);

/**
 * @brief Set the gadget device BCD USB version
 * @param g Pointer to gadget
 * @param bcdUSB BCD USB version
 */
extern void usbg_set_gadget_device_bcd_usb(struct gadget *g, uint16_t bcdUSB);

/**
 * @brief Get the USB gadget strings
 * @param g Pointer to gadget
 * @param g_sttrs Structure to be filled
 * @retur Pointer to filled structure or NULL if error occurred.
 */
extern struct gadget_strs *usbg_get_gadget_strs(struct gadget *g,
		struct gadget_strs *g_strs);

/**
 * @brief Set the USB gadget strings
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param g_sttrs Gadget attributes
 */
extern void usbg_set_gadget_strs(struct gadget *g, int lang,
		struct gadget_strs *g_strs);

/**
 * @brief Set the serial number for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param ser Serial number
 */
extern void usbg_set_gadget_serial_number(struct gadget *g, int lang, char *ser);

/**
 * @brief Set the manufacturer name for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param mnf Manufacturer
 */
extern void usbg_set_gadget_manufacturer(struct gadget *g, int lang, char *mnf);

/**
 * @brief Set the product name for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param prd Product
 */
extern void usbg_set_gadget_product(struct gadget *g, int lang, char *prd);

/* USB function allocation and configuration */

/**
 * @brief Create a new USB gadget function
 * @param g Pointer to gadget
 * @param type Type of function
 * @param instance Function instance name
 * @return Pointer to function or NULL if it cannot be created
 */
extern struct function *usbg_create_function(struct gadget *g, enum function_type type, char *instance);

/* USB configurations allocation and configuration */

/**
 * @brief Create a new USB gadget configuration
 * @param g Pointer to gadget
 * @param name Name of configuration
 * @return Pointer to configuration or NULL if it cannot be created
 */
extern struct config *usbg_create_config(struct gadget *g, char *name);

/**
 * @brief Set the configuration maximum power
 * @param c Pointer to config
 * @param bMaxPower Maximum power (in 2 mA units)
 */
extern void usbg_set_config_max_power(struct config *c, int bMaxPower);

/**
 * @brief Set the configuration bitmap attributes
 * @param c Pointer to config
 * @param bmAttributes Configuration characteristics
 */
extern void usbg_set_config_bm_attrs(struct config *c, int bmAttributes);

/**
 * @brief Set the configuration string
 * @param c Pointer to config
 * @param lang USB language ID
 * @param string Configuration description
 */
extern void usbg_set_config_string(struct config *c, int lang, char *string);

/**
 * @brief Add a function to a configuration
 * @param c Pointer to config
 * @param name Name of configuration function binding
 * @param f Pointer to function
 * @return 0 on success, -1 on failure.
 */
extern int usbg_add_config_function(struct config *c, char *name, struct function *f);

/* USB gadget setup and teardown */

/**
 * @brief Get a list of UDC devices on the system
 * @param udc_list Pointer to pointer to dirent pointer
 * @return Number of UDC devices on success, -1 on failure
 */
extern int usbg_get_udcs(struct dirent ***udc_list);

/**
 * @brief Enable a USB gadget device
 * @param g Pointer to gadget
 * @param udc Name of UDC to enable gadget
 */
extern void usbg_enable_gadget(struct gadget *g, char *udc);

/**
 * @brief Disable a USB gadget device
 * @param g Pointer to gadget
 */
extern void usbg_disable_gadget(struct gadget *g);

/**
 * @brief Get gadget name length
 * @param g Gadget which name length should be returned
 * @return Length of name string or -1 if error occurred.
 * @note If gadget isn't enabled on any udc returned size is 0.
 */
extern size_t usbg_get_gadget_udc_len(struct gadget *g);

/**
 * @brieg Get name of udc to which gadget is binded
 * @param b Pointer to gadget
 * @param buf Buffer where udc name should be copied
 * @param len Length of given buffer
 * @return Pointer to destination or NULL if error occurred.
 * @note If gadget isn't enabled on any udc returned string is empty.
 */
extern char *usbg_get_gadget_udc(struct gadget *g, char *buf, size_t len);

/*
 * USB function-specific attribute configuration
 */

/**
 * @brief Set USB function network device address
 * @param f Pointer to function
 * @param addr Pointer to Ethernet address
 */
extern void usbg_set_net_dev_addr(struct function *f, struct ether_addr *addr);

/**
 * @brief Set USB function network host address
 * @param f Pointer to function
 * @param addr Pointer to Ethernet address
 */
extern void usbg_set_net_host_addr(struct function *f, struct ether_addr *addr);

/**
 * @brief Set USB function network qmult
 * @param f Pointer to function
 * @param qmult Queue length multiplier
 */
extern void usbg_set_net_qmult(struct function *f, int qmult);

/**
 * @def usbg_for_each_gadget(g, s)
 * Iterates over each gadget
 */
#define usbg_for_each_gadget(g, s) \
	for (g = usbg_get_first_gadget(s); \
	g != NULL; \
	g = usbg_get_next_gadget(g))

/**
 * @def usbg_for_each_function(f, g)
 * Iterates over each function
 */
#define usbg_for_each_function(f, g) \
	for (f = usbg_get_first_function(g); \
	f != NULL; \
	f = usbg_get_next_function(f))

/**
 * @def usbg_for_each_config(c, g)
 * Iterates over each config
 */
#define usbg_for_each_config(c, g) \
	for (c = usbg_get_first_config(g); \
	c != NULL; \
	c = usbg_get_next_config(c))

/**
 * @def usbg_for_each_binding(b, c)
 * Iterates over each binding
 */
#define usbg_for_each_binding(b, c)	\
	for (b = usbg_get_first_binding(c); \
	b != NULL; \
	b = usbg_get_next_binding(b))

/**
 * @brief Get first gadget in gadget list
 * @param s State of library
 * @return Pointer to gadget or NULL if list is empty.
 * @note Gadgets are sorted in strings (name) order
 */
extern struct gadget *usbg_get_first_gadget(struct state *s);

/**
 * @brief Get first function in function list
 * @param g Pointer of gadget
 * @return Pointer to function or NULL if list is empty.
 * @note Functions are sorted in strings (name) order
 */
extern struct function *usbg_get_first_function(struct gadget *g);

/**
 * @brief Get first config in config list
 * @param g Pointer of gadget
 * @return Pointer to configuration or NULL if list is empty.
 * @note Configs are sorted in strings (name) order
 */
extern struct config *usbg_get_first_config(struct gadget *g);

/**
 * @brief Get first binding in binding list
 * @param C Pointer to configuration
 * @return Pointer to binding or NULL if list is empty.
 * @note Bindings are sorted in strings (name) order
 */
extern struct binding *usbg_get_first_binding(struct config *c);

/**
 * @brief Get the next gadget on a list.
 * @pram g Pointer to current gadget
 * @return Next gadget or NULL if end of list.
 */
extern struct gadget *usbg_get_next_gadget(struct gadget *g);

/**
 * @brief Get the next function on a list.
 * @pram g Pointer to current function
 * @return Next function or NULL if end of list.
 */
extern struct function *usbg_get_next_function(struct function *f);

/**
 * @brief Get the next config on a list.
 * @pram g Pointer to current config
 * @return Next config or NULL if end of list.
 */
extern struct config *usbg_get_next_config(struct config *c);

/**
 * @brief Get the next binding on a list.
 * @pram g Pointer to current binding
 * @return Next binding or NULL if end of list.
 */
extern struct binding *usbg_get_next_binding(struct binding *b);

/**
 * @}
 */
#endif /* __USBG_H__ */
