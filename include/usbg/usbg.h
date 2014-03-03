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

/*
 * Internal structures
 */
struct usbg_state;
struct usbg_gadget;
struct usbg_config;
struct usbg_function;
struct usbg_binding;

/**
 * @brief State of the gadget devices in the system
 */
typedef struct usbg_state usbg_state;

/**
 * @brief USB gadget device
 */
typedef struct usbg_gadget usbg_gadget;

/**
 * @brief USB configuration
 */
typedef struct usbg_config usbg_config;

/**
 * @brief USB function
 */
typedef struct usbg_function usbg_function;

/**
 * @brief USB function to config binding
 */
typedef struct usbg_binding usbg_binding;

/**
 * @typedef usbg_gadget_attrs
 * @brief USB gadget device attributes
 */
typedef struct
{
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
} usbg_gadget_attrs;

/**
 * @typedef usbg_gadget_strs
 * @brief USB gadget device strings
 */
typedef struct
{
	char str_ser[USBG_MAX_STR_LENGTH];
	char str_mnf[USBG_MAX_STR_LENGTH];
	char str_prd[USBG_MAX_STR_LENGTH];
} usbg_gadget_strs;

/**
 * @typedef usbg_config_attrs
 * @brief USB configuration attributes
 */
typedef struct
{
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} usbg_config_attrs;

/**
 * @typedef usbg_config_strs
 * @brief USB configuration strings
 */
typedef struct
{
	char configuration[USBG_MAX_STR_LENGTH];
} usbg_config_strs;

/**
 * @typedef usbg_function_type
 * @brief Supported USB function types
 */
typedef enum
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
} usbg_function_type;

/**
 * @typedef usbg_f_serial_attrs
 * @brief Attributes for Serial, ACM, and OBEX USB functions
 */
typedef struct {
	int port_num;
} usbg_f_serial_attrs;

/**
 * @typedef net_attrs
 * @brief Attributes for ECM, ECM subset, NCM, EEM, and RNDIS USB functions
 */
typedef struct {
	struct ether_addr dev_addr;
	struct ether_addr host_addr;
	char ifname[USBG_MAX_STR_LENGTH];
	int qmult;
} usbg_f_net_attrs;

/**
 * @typedef usbg_f_phonet_attrs
 * @brief Attributes for the phonet USB function
 */
typedef struct {
	char ifname[USBG_MAX_STR_LENGTH];
} usbg_f_phonet_attrs;

/**
 * @typedef attrs
 * @brief Attributes for a given function type
 */
typedef union {
	usbg_f_serial_attrs serial;
	usbg_f_net_attrs net;
	usbg_f_phonet_attrs phonet;
} usbg_function_attrs;

/**
 * @typedef usbg_error
 * @brief Errors which could be returned by library functions
 */
typedef enum  {
	USBG_SUCCESS = 0,
	USBG_ERROR_NO_MEM = -1,
	USBG_ERROR_NO_ACCESS = -2,
	USBG_ERROR_INVALID_PARAM = -3,
	USBG_ERROR_NOT_FOUND = -4,
	USBG_ERROR_IO = -5,
	USBG_ERROR_EXIST = -6,
	USBG_ERROR_NO_DEV = -7,
	USBG_ERROR_BUSY = -8,
	USBG_ERROR_OTHER_ERROR = -99
} usbg_error;

/* Library init and cleanup */

/**
 * @brief Initialize the libusbgx library state
 * @param configfs_path Path to the mounted configfs filesystem
 * @param Pointer to be filled with pointer to usbg_state
 * @return 0 on success, usbg_error on error
 */
extern int usbg_init(char *configfs_path, usbg_state **state);

/**
 * @brief Clean up the libusbgx library state
 * @param s Pointer to state
 */
extern void usbg_cleanup(usbg_state *s);

/**
 * @brief Get ConfigFS path length
 * @param s Pointer to state
 * @return Length of path or usbg_error if error occurred.
 */
extern size_t usbg_get_configfs_path_len(usbg_state *s);

/**
 * @brieg Get ConfigFS path
 * @param s Pointer to state
 * @param buf Buffer where path should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_get_configfs_path(usbg_state *s, char *buf, size_t len);

/* USB gadget queries */

/**
 * @brief Get a gadget device by name
 * @param s Pointer to state
 * @param name Name of the gadget device
 * @return Pointer to gadget or NULL if a matching gadget isn't found
 */
extern usbg_gadget *usbg_get_gadget(usbg_state *s, const char *name);

/**
 * @brief Get a function by name
 * @param g Pointer to gadget
 * @param name Name of the function
 * @return Pointer to function or NULL if a matching function isn't found
 */
extern usbg_function *usbg_get_function(usbg_gadget *g, const char *name);

/**
 * @brief Get a configuration by name
 * @param g Pointer to gadget
 * @param name Name of the configuration
 * @return Pointer to config or NULL if a matching config isn't found
 */
extern usbg_config *usbg_get_config(usbg_gadget *g, const char *name);

/* USB gadget allocation and configuration */

/**
 * @brief Create a new USB gadget device
 * @param s Pointer to state
 * @param name Name of the gadget
 * @param idVendor Gadget vendor ID
 * @param idProduct Gadget product ID
 * @param g Pointer to be filled with pointer to gadget
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_gadget_vid_pid(usbg_state *s, char *name,
		uint16_t idVendor, uint16_t idProduct, usbg_gadget **g);

/**
 * @brief Create a new USB gadget device and set given attributes
 * and strings
 * @param s Pointer to state
 * @param name Name of the gadget
 * @param g_attrs Gadget attributes to be set. If NULL setting is omitted.
 * @param g_strs Gadget strings to be set. If NULL setting is omitted.
 * @param g Pointer to be filled with pointer to gadget
 * @note Given strings are assumed to be in US English
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_gadget(usbg_state *s, char *name,
		usbg_gadget_attrs *g_attrs, usbg_gadget_strs *g_strs, usbg_gadget **g);

/**
 * @brief Set the USB gadget attributes
 * @param g Pointer to gadget
 * @param g_attrs Gadget attributes
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_attrs(usbg_gadget *g,
		usbg_gadget_attrs *g_attrs);

/**
 * @brief Get the USB gadget strings
 * @param g Pointer to gadget
 * @param g_attrs Structure to be filled
 * @retur Pointer to filled structure or NULL if error occurred.
 */
extern usbg_gadget_attrs *usbg_get_gadget_attrs(usbg_gadget *g,
		usbg_gadget_attrs *g_attrs);

/**
 * @brief Get gadget name length
 * @param g Gadget which name length should be returned
 * @return Length of name string or usbg_error if error occurred.
 */
extern size_t usbg_get_gadget_name_len(usbg_gadget *g);

/**
 * @brieg Get gadget name
 * @param b Pointer to gadget
 * @param buf Buffer where name should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_get_gadget_name(usbg_gadget *g, char *buf, size_t len);

/**
 * @brief Set the USB gadget vendor id
 * @param g Pointer to gadget
 * @param idVendor USB device vendor id
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_vendor_id(usbg_gadget *g, uint16_t idVendor);

/**
 * @brief Set the USB gadget product id
 * @param g Pointer to gadget
 * @param idProduct USB device product id
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_product_id(usbg_gadget *g, uint16_t idProduct);

/**
 * @brief Set the USB gadget device class code
 * @param g Pointer to gadget
 * @param bDeviceClass USB device class code
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_class(usbg_gadget *g,
		uint8_t bDeviceClass);

/**
 * @brief Set the USB gadget protocol code
 * @param g Pointer to gadget
 * @param bDeviceProtocol USB protocol code
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_protocol(usbg_gadget *g,
		uint8_t bDeviceProtocol);

/**
 * @brief Set the USB gadget device subclass code
 * @param g Pointer to gadget
 * @param bDeviceSubClass USB device subclass code
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_subclass(usbg_gadget *g,
		uint8_t bDeviceSubClass);

/**
 * @brief Set the maximum packet size for a gadget
 * @param g Pointer to gadget
 * @param bMaxPacketSize0 Maximum packet size
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_max_packet(usbg_gadget *g,
		uint8_t bMaxPacketSize0);

/**
 * @brief Set the gadget device BCD release number
 * @param g Pointer to gadget
 * @param bcdDevice BCD release number
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_bcd_device(usbg_gadget *g,
		uint16_t bcdDevice);

/**
 * @brief Set the gadget device BCD USB version
 * @param g Pointer to gadget
 * @param bcdUSB BCD USB version
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_device_bcd_usb(usbg_gadget *g, uint16_t bcdUSB);

/**
 * @brief Get the USB gadget strings
 * @param g Pointer to gadget
 * @param lang Language of strings
 * @param g_sttrs Structure to be filled
 * @retur Pointer to filled structure or NULL if error occurred or
 * if no strings for given language.
 */
extern usbg_gadget_strs *usbg_get_gadget_strs(usbg_gadget *g, int lang,
		usbg_gadget_strs *g_strs);

/**
 * @brief Set the USB gadget strings
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param g_sttrs Gadget attributes
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_strs(usbg_gadget *g, int lang,
		usbg_gadget_strs *g_strs);

/**
 * @brief Set the serial number for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param ser Serial number
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_serial_number(usbg_gadget *g, int lang, char *ser);

/**
 * @brief Set the manufacturer name for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param mnf Manufacturer
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_manufacturer(usbg_gadget *g, int lang, char *mnf);

/**
 * @brief Set the product name for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param prd Product
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_product(usbg_gadget *g, int lang, char *prd);

/* USB function allocation and configuration */

/**
 * @brief Create a new USB gadget function and set its attributes
 * @param g Pointer to gadget
 * @param type Type of function
 * @param instance Function instance name
 * @param f_attrs Function attributes to be set. If NULL setting is omitted.
 * @param f Pointer to be filled with pointer to function
 * @note Given strings are assumed to be in US English
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_function(usbg_gadget *g, usbg_function_type type,
		char *instance, usbg_function_attrs *f_attrs, usbg_function **f);

/**
 * @brief Get function name length
 * @param f Config which name length should be returned
 * @return Length of name string or usbg_error if error occurred.
 */
extern size_t usbg_get_function_name_len(usbg_function *f);

/**
 * @brieg Get config name
 * @param f Pointer to function
 * @param buf Buffer where name should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_get_function_name(usbg_function *f, char *buf, size_t len);

/* USB configurations allocation and configuration */

/**
 * @brief Create a new USB gadget configuration
 * @param g Pointer to gadget
 * @param name Name of configuration
 * @param c_attrs Configuration attributes to be set
 * @param c_strs Configuration strings to be set
 * @param c Pointer to be filled with pointer to configuration
 * @note Given strings are assumed to be in US English
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_config(usbg_gadget *g, char *name,
		usbg_config_attrs *c_attrs, usbg_config_strs *c_strs, usbg_config **c);

/**
 * @brief Get config name length
 * @param c Config which name length should be returned
 * @return Length of name string or usbg_error if error occurred.
 */
extern size_t usbg_get_config_name_len(usbg_config *c);

/**
 * @brieg Get config name
 * @param c Pointer to config
 * @param buf Buffer where name should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_get_config_name(usbg_config *c, char *buf, size_t len);

/**
 * @brief Set the USB configuration attributes
 * @param c Pointer to configuration
 * @param c_attrs Configuration attributes
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_set_config_attrs(usbg_config *c,
		usbg_config_attrs *c_attrs);

/**
 * @brief Get the USB configuration strings
 * @param c Pointer to configuration
 * @param c_attrs Structure to be filled
 * @retur Pointer to filled structure or NULL if error occurred.
 */
extern usbg_config_attrs *usbg_get_config_attrs(usbg_config *c,
		usbg_config_attrs *c_attrs);

/**
 * @brief Set the configuration maximum power
 * @param c Pointer to config
 * @param bMaxPower Maximum power (in 2 mA units)
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_set_config_max_power(usbg_config *c, int bMaxPower);

/**
 * @brief Set the configuration bitmap attributes
 * @param c Pointer to config
 * @param bmAttributes Configuration characteristics
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_set_config_bm_attrs(usbg_config *c, int bmAttributes);

/**
 * @brief Get the USB configuration strings
 * @param c Pointer to configuration
 * @param lang Language of strings
 * @param c_sttrs Structure to be filled
 * @retur Pointer to filled structure or NULL if error occurred.
 */
extern usbg_config_strs *usbg_get_config_strs(usbg_config *c, int lang,
		usbg_config_strs *c_strs);

/**
 * @brief Set the USB configuration strings
 * @param c Pointer to configuration
 * @param lang USB language ID
 * @param c_sttrs Configuration strings
 * @return 0 on success, usbg_error on failure.
 */
extern int usbg_set_config_strs(usbg_config *c, int lang,
		usbg_config_strs *c_strs);

/**
 * @brief Set the configuration string
 * @param c Pointer to config
 * @param lang USB language ID
 * @param string Configuration description
 * @return 0 on success, usbg_error on failure.
 */
extern int usbg_set_config_string(usbg_config *c, int lang, char *string);

/**
 * @brief Add a function to a configuration
 * @param c Pointer to config
 * @param name Name of configuration function binding
 * @param f Pointer to function
 * @return 0 on success, -1 on failure.
 */
extern int usbg_add_config_function(usbg_config *c, char *name, usbg_function *f);

/**
 * @brief Get target function of given binding
 * @param b Binding between configuration and function
 * @return Pointer to USB function which is target for this binding
 */
extern usbg_function *usbg_get_binding_target(usbg_binding *b);

/**
 * @brief Get binding name length
 * @param b Binding which name length should be returned
 * @return Length of name string or usbg_error if error occurred.
 */
extern size_t usbg_get_binding_name_len(usbg_binding *b);

/**
 * @brief Get binding name
 * @param b Pointer to binding
 * @param buf Buffer where name should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_get_binding_name(usbg_binding *b, char *buf, size_t len);

/* USB gadget setup and teardown */

/**
 * @brief Get a list of UDC devices on the system
 * @param udc_list Pointer to pointer to dirent pointer
 * @return Number of UDC devices on success, usbg_error on failure
 */
extern int usbg_get_udcs(struct dirent ***udc_list);

/**
 * @brief Enable a USB gadget device
 * @param g Pointer to gadget
 * @param udc Name of UDC to enable gadget
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_enable_gadget(usbg_gadget *g, char *udc);

/**
 * @brief Disable a USB gadget device
 * @param g Pointer to gadget
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_disable_gadget(usbg_gadget *g);

/**
 * @brief Get gadget name length
 * @param g Gadget which name length should be returned
 * @return Length of name string or usbg_error if error occurred.
 * @note If gadget isn't enabled on any udc returned size is 0.
 */
extern size_t usbg_get_gadget_udc_len(usbg_gadget *g);

/**
 * @brieg Get name of udc to which gadget is binded
 * @param b Pointer to gadget
 * @param buf Buffer where udc name should be copied
 * @param len Length of given buffer
 * @return 0 on success or usbg_error if error occurred.
 * @note If gadget isn't enabled on any udc returned string is empty.
 */
extern int usbg_get_gadget_udc(usbg_gadget *g, char *buf, size_t len);

/*
 * USB function-specific attribute configuration
 */

/**
 * @brief Get type of given function
 * @param f Pointer to function
 * @return Type of function
 * @warning Pointer to function has to be valid.
 */
extern usbg_function_type usbg_get_function_type(usbg_function *f);

/**
 * @brief Get attributes of given function
 * @param f Pointer to function
 * @param f_attrs Union to be filled
 * @return Pointer to filled structure or NULL if error occurred.
 */
extern usbg_function_attrs *usbg_get_function_attrs(usbg_function *f,
		usbg_function_attrs *f_attrs);

/**
 * @brief Set attributes of given function
 * @param f Pointer to function
 * @param f_attrs Attributes to be set
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_set_function_attrs(usbg_function *f, usbg_function_attrs *f_attrs);

/**
 * @brief Set USB function network device address
 * @param f Pointer to function
 * @param addr Pointer to Ethernet address
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_set_net_dev_addr(usbg_function *f, struct ether_addr *addr);

/**
 * @brief Set USB function network host address
 * @param f Pointer to function
 * @param addr Pointer to Ethernet address
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_set_net_host_addr(usbg_function *f, struct ether_addr *addr);

/**
 * @brief Set USB function network qmult
 * @param f Pointer to function
 * @param qmult Queue length multiplier
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_set_net_qmult(usbg_function *f, int qmult);

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
extern usbg_gadget *usbg_get_first_gadget(usbg_state *s);

/**
 * @brief Get first function in function list
 * @param g Pointer of gadget
 * @return Pointer to function or NULL if list is empty.
 * @note Functions are sorted in strings (name) order
 */
extern usbg_function *usbg_get_first_function(usbg_gadget *g);

/**
 * @brief Get first config in config list
 * @param g Pointer of gadget
 * @return Pointer to configuration or NULL if list is empty.
 * @note Configs are sorted in strings (name) order
 */
extern usbg_config *usbg_get_first_config(usbg_gadget *g);

/**
 * @brief Get first binding in binding list
 * @param C Pointer to configuration
 * @return Pointer to binding or NULL if list is empty.
 * @note Bindings are sorted in strings (name) order
 */
extern usbg_binding *usbg_get_first_binding(usbg_config *c);

/**
 * @brief Get the next gadget on a list.
 * @pram g Pointer to current gadget
 * @return Next gadget or NULL if end of list.
 */
extern usbg_gadget *usbg_get_next_gadget(usbg_gadget *g);

/**
 * @brief Get the next function on a list.
 * @pram g Pointer to current function
 * @return Next function or NULL if end of list.
 */
extern usbg_function *usbg_get_next_function(usbg_function *f);

/**
 * @brief Get the next config on a list.
 * @pram g Pointer to current config
 * @return Next config or NULL if end of list.
 */
extern usbg_config *usbg_get_next_config(usbg_config *c);

/**
 * @brief Get the next binding on a list.
 * @pram g Pointer to current binding
 * @return Next binding or NULL if end of list.
 */
extern usbg_binding *usbg_get_next_binding(usbg_binding *b);

/**
 * @}
 */
#endif /* __USBG_H__ */
