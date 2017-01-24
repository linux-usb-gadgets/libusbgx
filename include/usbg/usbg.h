/*
 * Copyright (C) 2013 Linaro Limited
 *
 * Matt Porter <mporter@linaro.org>
 *
 * Copyright (C) 2013 - 2015 Samsung Electronics
 *
 * Krzysztof Opasiak <k.opasiak@samsung.com>
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
#include <limits.h>
#include <stdbool.h>
#include <stdio.h> /* For FILE * */
#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file include/usbg/usbg.h
 * @todo Clean up static buffers in structures
 */

/**
 * @addtogroup libusbgx
 * Public API for USB gadget-configfs library
 * @{
 */

#define DEFAULT_UDC		NULL
#define LANG_US_ENG		0x0409
#define DEFAULT_CONFIG_LABEL "config"

/* This one has to be at least 18 bytes to hold network address */
#define USBG_MAX_STR_LENGTH 256
#define USBG_MAX_NAME_LENGTH 40
/* Dev name for ffs is a part of function name, we subtract 4 char for "ffs." */
#define USBG_MAX_DEV_LENGTH (USBG_MAX_NAME_LENGTH - 4)

/**
 * @brief Additional option for usbg_rm_* functions.
 * @details This option allows to remove all content
 * of gadget/config/function recursively.
 */
#define USBG_RM_RECURSE 1

/*
 * Internal structures
 */
struct usbg_state;
struct usbg_gadget;
struct usbg_config;
struct usbg_function;
struct usbg_binding;
struct usbg_udc;

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
 * @brief USB device controller
 */
typedef struct usbg_udc usbg_udc;

/**
 * @typedef usbg_gadget_attr
 * @brief Gadget attributes which can be set using
 * usbg_set_gadget_attr() function.
 */
typedef enum {
	USBG_GADGET_ATTR_MIN = 0,
	USBG_BCD_USB = USBG_GADGET_ATTR_MIN,
	USBG_B_DEVICE_CLASS,
	USBG_B_DEVICE_SUB_CLASS,
	USBG_B_DEVICE_PROTOCOL,
	USBG_B_MAX_PACKET_SIZE_0,
	USBG_ID_VENDOR,
	USBG_ID_PRODUCT,
	USBG_BCD_DEVICE,
	USBG_GADGET_ATTR_MAX,
} usbg_gadget_attr;

/**
 * @brief USB gadget device attributes
 */
struct usbg_gadget_attrs
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

typedef enum {
	USBG_GADGET_STR_MIN = 0,
	USBG_STR_MANUFACTURER = USBG_GADGET_STR_MIN,
	USBG_STR_PRODUCT,
	USBG_STR_SERIAL_NUMBER,
	USBG_GADGET_STR_MAX,
} usbg_gadget_str;

/**
 * @brief USB gadget device strings
 */
struct usbg_gadget_strs
{
	char *manufacturer;
	char *product;
	char *serial;
};

/**
 * @brief USB gadget Microsoft OS Descriptors
 */
struct usbg_gadget_os_descs
{
	bool use;
	uint8_t b_vendor_code;
	char *qw_sign;
};

/**
 * @typedef usbg_gadget_os_desc_strs
 * @brief Microsoft OS Descriptors strings
 */
typedef enum {
	USBG_GADGET_OS_DESC_MIN = 0,
	OS_DESC_USE = USBG_GADGET_OS_DESC_MIN,
	OS_DESC_B_VENDOR_CODE,
	OS_DESC_QW_SIGN,
	USBG_GADGET_OS_DESC_MAX,
} usbg_gadget_os_desc_strs;

/**
 * @brief USB configuration attributes
 */
struct usbg_config_attrs
{
	uint8_t bmAttributes;
	uint8_t bMaxPower;
};

/**
 * @brief USB configuration strings
 */
struct usbg_config_strs
{
	char *configuration;
};

/**
 * @typedef usbg_function_type
 * @brief Supported USB function types
 */
typedef enum
{
	USBG_FUNCTION_TYPE_MIN = 0,
	USBG_F_SERIAL = USBG_FUNCTION_TYPE_MIN,
	USBG_F_ACM,
	USBG_F_OBEX,
	USBG_F_ECM,
	USBG_F_SUBSET,
	USBG_F_NCM,
	USBG_F_EEM,
	USBG_F_RNDIS,
	USBG_F_PHONET,
	USBG_F_FFS,
	USBG_F_MASS_STORAGE,
	USBG_F_MIDI,
	USBG_F_LOOPBACK,
	USBG_F_HID,
	USBG_FUNCTION_TYPE_MAX,
} usbg_function_type;

/* Error codes */

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
	USBG_ERROR_NOT_SUPPORTED = -9,
	USBG_ERROR_PATH_TOO_LONG = -10,
	USBG_ERROR_INVALID_FORMAT = -11,
	USBG_ERROR_MISSING_TAG = -12,
	USBG_ERROR_INVALID_TYPE = -13,
	USBG_ERROR_INVALID_VALUE = -14,
	USBG_ERROR_NOT_EMPTY = -15,
	USBG_ERROR_OTHER_ERROR = -99
} usbg_error;

/**
 * @brief Get the error name as a constant string
 * @param e error code
 * @return Constant string with error name
 */
extern const char *usbg_error_name(usbg_error e);

/**
 * @brief Get the short description of error
 * @param e error code
 * @return Constant string with error description
 */
extern const char *usbg_strerror(usbg_error e);

/* Library init and cleanup */

/**
 * @brief Initialize the libusbgx library state
 * @param configfs_path Path to the mounted configfs filesystem
 * @param state Pointer to be filled with pointer to usbg_state
 * @return 0 on success, usbg_error on error
 */
extern int usbg_init(const char *configfs_path, usbg_state **state);

/**
 * @brief Clean up the libusbgx library state
 * @param s Pointer to state
 */
extern void usbg_cleanup(usbg_state *s);

/**
 * @brief Get ConfigFS path
 * @param s Pointer to state
 * @return Path to configfs or NULL if error occurred
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_state is valid.
 * For example path is valid until usbg_cleanup() call.
 */
extern const char *usbg_get_configfs_path(usbg_state *s);

/**
 * @brief Get ConfigFS path into user buffer
 * @param[in] s Pointer to state
 * @param[out] buf Place where path should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family).
 */
extern int usbg_get_configfs_path_s(usbg_state *s, char *buf, int len);

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
 * @param type Function type
 * @param instance Instance of function
 * @return Pointer to function or NULL if a matching function isn't found
 */
extern usbg_function *usbg_get_function(usbg_gadget *g,
		usbg_function_type type, const char *instance);

/**
 * @brief Get a configuration by name
 * @param g Pointer to gadget
 * @param id Identify of configuration
 * @param label Configuration label. If not NULL this function will return
 * valid value only if configuration with given id exist and has this label.
 * If NULL this function will return config with given id (if such exist)
 * and ignore this param.
 * @return Pointer to config or NULL if a matching config isn't found
 */
extern usbg_config *usbg_get_config(usbg_gadget *g, int id, const char *label);

/**
 * @brief Get a udc by name
 * @param s Pointer to state
 * @param name Name of the udc
 * @return Pointer to udc or NULL if a matching udc isn't found
 */
extern usbg_udc *usbg_get_udc(usbg_state *s, const char *name);

/* USB gadget/config/function/binding removal */

/**
 * @brief Remove binding between configuration and function
 * @details This function frees also the memory allocated for binding
 * @param b Binding to be removed
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_binding(usbg_binding *b);

/**
 * @brief Remove configuration
 * @details This function frees also the memory allocated for configuration
 * @param c Configuration to be removed
 * @param opts Additional options for configuration removal.
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_config(usbg_config *c, int opts);

/**
 * @brief Remove existing USB function
 * @details This function frees also the memory allocated for function
 * @param f Function to be removed
 * @param opts Additional options for configuration removal.
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_function(usbg_function *f, int opts);

/**
 * @brief Remove existing USB gadget
 * @details This function frees also the memory allocated for gadget
 * @param g Gadget to be removed
 * @param opts Additional options for configuration removal.
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_gadget(usbg_gadget *g, int opts);

/**
 * @brief Remove configuration strings for given language
 * @param c Pointer to configuration
 * @param lang Language of strings which should be deleted
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_config_strs(usbg_config *c, int lang);

/**
 * @brief Remove gadget strings for given language
 * @param g Pointer to gadget
 * @param lang Language of strings which should be deleted
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_rm_gadget_strs(usbg_gadget *g, int lang);

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
extern int usbg_create_gadget_vid_pid(usbg_state *s, const char *name,
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
extern int usbg_create_gadget(usbg_state *s, const char *name,
			      const struct usbg_gadget_attrs *g_attrs,
			      const struct usbg_gadget_strs *g_strs,
			      usbg_gadget **g);

/**
 * @brief Get string representing selected gadget attribute
 * @param attr code of selected attribute
 * @return String suitable for given attribute or NULL if such
 * string has not been found
 */
extern const char *usbg_get_gadget_attr_str(usbg_gadget_attr attr);

/**
 * @brief Lookup attr code based on its name
 * @param name of attribute
 * @return code of suitable attribute or usbg_error
 */
extern int usbg_lookup_gadget_attr(const char *name);

/**
 * @brief Lookup str code based on its name
 * @param name of string
 * @return code of suitable string or usbg_error
 */
extern int usbg_lookup_gadget_str(const char *name);

/**
 * @brief Get name of selected gadget string
 * @param str Gadget string code
 * @return Name of string associated with this code
 */
extern const char *usbg_get_gadget_str_name(usbg_gadget_str str);

/**
 * @brief Get name of selected OS Descriptor string
 * @param str OS Descriptor string code
 * @return Name of OS Descriptor associated with this code
 */
extern const char *usbg_get_gadget_os_desc_name(usbg_gadget_os_desc_strs str);

/**
 * @brief Set selected attribute to value
 * @param g Pointer to gadget
 * @param attr Code of selected attribute
 * @param val value to be set
 * @return 0 on success, usbg_error otherwise
 * @note val is of type int but value provided to this function should
 * be suitable to place it in type dedicated for selected attr (uint16 or uint8)
 */
extern int usbg_set_gadget_attr(usbg_gadget *g, usbg_gadget_attr attr, int val);

/**
 * @brief Get value of selected attribute
 * @param g Pointer to gadget
 * @param attr Code of selected attribute
 * @return Value of selected attribute (always above zero) or
 * usbg_error if error occurred.
 * @note User should use only lowest one or two bytes as attribute value
 * depending on attribute size (see usbg_gadget_attrs for details).
 */
extern int usbg_get_gadget_attr(usbg_gadget *g, usbg_gadget_attr attr);

/**
 * @brief Set the USB gadget attributes
 * @param g Pointer to gadget
 * @param g_attrs Gadget attributes
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_attrs(usbg_gadget *g,
				 const struct usbg_gadget_attrs *g_attrs);

/**
 * @brief Get the USB gadget strings
 * @param g Pointer to gadget
 * @param g_attrs Structure to be filled
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_get_gadget_attrs(usbg_gadget *g,
				 struct usbg_gadget_attrs *g_attrs);

/**
 * @brief Get gadget name
 * @param g Pointer to gadget
 * @return Gadget name or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_gadget is valid.
 * For example gadget name is valid until someone remove gadget.
 */
extern const char *usbg_get_gadget_name(usbg_gadget *g);

/**
 * @brief Get gadget name into user buffer
 * @param[in] g Pointer to state
 * @param[out] buf Place where name should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family).
 */
extern int usbg_get_gadget_name_s(usbg_gadget *g, char *buf, int len);

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
 * @param g_strs Structure to be filled
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_get_gadget_strs(usbg_gadget *g, int lang,
				struct usbg_gadget_strs *g_strs);

/**
 * @brief Get the array of languages available in this gadget
 * @param g Pointer to gadget
 * @param langs array of available language codes
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_get_gadget_strs_langs(usbg_gadget *g, int **langs);

/**
 * @brief Free gadget strings
 * @details This function releases the memory allocated for strings
 *          not for struct usbg_gadget_strs itself.
 * @param g_strs Strings to be released
 */
static inline void usbg_free_gadget_strs(struct usbg_gadget_strs *g_strs)
{
	if (!g_strs)
		return;

	free(g_strs->manufacturer);
	free(g_strs->product);
	free(g_strs->serial);
}

/**
 * @brief Set selected string
 * @param g Pointer to gadget
 * @param str Code of selected string
 * @param val value to be set
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_set_gadget_str(usbg_gadget *g, usbg_gadget_str str, int lang,
		const char *val);

/**
 * @brief Set the USB gadget strings
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param g_strs Gadget attributes
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_strs(usbg_gadget *g, int lang,
				const struct usbg_gadget_strs *g_strs);

/**
 * @brief Set the serial number for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param ser Serial number
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_serial_number(usbg_gadget *g, int lang,
					 const char *ser);

/**
 * @brief Set the manufacturer name for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param mnf Manufacturer
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_manufacturer(usbg_gadget *g, int lang,
					const char *mnf);

/**
 * @brief Set the product name for a gadget
 * @param g Pointer to gadget
 * @param lang USB language ID
 * @param prd Product
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_set_gadget_product(usbg_gadget *g, int lang,
				   const char *prd);

/**
 * @brief Get the USB gadget OS Descriptor
 * @param g Pointer to gadget
 * @param g_os_descs Structure to be filled
 * @return 0 on success usbg_error if error occurred
 */

extern int usbg_get_gadget_os_descs(usbg_gadget *g,
		struct usbg_gadget_os_descs *g_os_descs);

/**
 * @brief Free OS Descriptor attributes
 * @details This function releases the memory allocated for USB
 *          gadget OS Descriptor atrributes.
 * @param g_os_desc OS Descriptor attributes to be released
 */
static inline void usbg_free_gadget_os_desc(
			struct usbg_gadget_os_descs *g_os_desc)
{
	if (!g_os_desc)
		return;

	free(g_os_desc->qw_sign);
}

/**
 * @brief Set the USB gadget OS Descriptor
 * @param g Pointer to gadget
 * @param g_os_descs Structure to be filled
 * @return 0 on success usbg_error if error occurred
 */

extern int usbg_set_gadget_os_descs(usbg_gadget *g,
		const struct usbg_gadget_os_descs *g_os_descs);

/* USB function allocation and configuration */

/**
 * @brief Create a new USB gadget function and set its attributes
 * @param g Pointer to gadget
 * @param type Type of function
 * @param instance Function instance name
 * @param f_attrs Function specific attributes to be set.
 *        If NULL setting is omitted.
 * @param f Pointer to be filled with pointer to function
 * @note Given strings are assumed to be in US English
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_function(usbg_gadget *g, usbg_function_type type,
		 const char *instance, void *f_attrs, usbg_function **f);

/**
 * @brief Get function instance name
 * @param f Pointer to function
 * @return instance name or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_function is valid.
 * For example instance name is valid until someone remove this function.
 */
extern const char *usbg_get_function_instance(usbg_function *f);

/**
 * @brief Get function instance name into user buffer
 * @param[in] f Pointer to function
 * @param[out] buf Place where instance name should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family).
 */
extern int usbg_get_function_instance_s(usbg_function *f, char *buf, int len);

/**
 * @brief Get function type as a string
 * @param type Function type
 * @return String suitable for given function type
 */
extern const char *usbg_get_function_type_str(usbg_function_type type);

/**
 * @brief Lookup function type suitable for given name
 * @param name Name of function
 * @return Function type enum or negative error code
 */
extern int usbg_lookup_function_type(const char *name);

/**
 * @brief Cleanup content of function attributes
 * @param f_attrs function attributes which should be cleaned up.
 * @note This function should be called to free
 * additional memory allocated by usbg_get_function_attrs().
 * @warning None of attributes in passed structure should be
 * accessed after returning from this function.
 */
extern void usbg_cleanup_function_attrs(usbg_function *f, void *f_attrs);

/**
 * @brief Get type of given function
 * @param f Pointer to function
 * @return usbg_function_type (0 or above) or
 *  usbg_error (below 0) if error occurred
 */
extern usbg_function_type usbg_get_function_type(usbg_function *f);

/**
 * @brief Get attributes of given function
 * @param f Pointer to function
 * @param f_attrs Union to be filled
 * @return 0 on success usbg_error if error occurred.
 * @warning memory pointed by f_attrs should be big enough to hold attributes
 *         specific for given function type. This function can by dangerous.
 *         That's why it is strongly recomended to use set/get function provided
 *         by each function type.
 */
extern int usbg_get_function_attrs(usbg_function *f, void *f_attrs);

/**
 * @brief Set attributes of given function
 * @param f Pointer to function
 * @param f_attrs Attributes to be set
 * @return 0 on success, usbg_error if error occurred
 */
extern int usbg_set_function_attrs(usbg_function *f, void *f_attrs);

/* USB configurations allocation and configuration */

/**
 * @brief Create a new USB gadget configuration
 * @param g Pointer to gadget
 * @param id Identify of configuration
 * @param label configuration label, if NULL, default is used
 * @param c_attrs Configuration attributes to be set
 * @param c_strs Configuration strings to be set
 * @param c Pointer to be filled with pointer to configuration
 * @note Given strings are assumed to be in US English
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_create_config(usbg_gadget *g, int id, const char *label,
			      const struct usbg_config_attrs *c_attrs,
			      const struct usbg_config_strs *c_strs,
			      usbg_config **c);

/**
 * @brief Get config label
 * @param c Pointer to config
 * @return config label or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_config is valid.
 * For example config label is valid until someone remove this function.
 */
extern const char *usbg_get_config_label(usbg_config *c);

/**
 * @brief Get config label into user buffer
 * @param[in] c Pointer to config
 * @param[out] buf Place where label should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family).
 */
extern int usbg_get_config_label_s(usbg_config *c, char *buf, int len);

/**
 * @brief Get config id
 * @param c Pointer to config
 * @return Configuration id or usbg_error if error occurred.
 */
extern int usbg_get_config_id(usbg_config *c);

/**
 * @brief Set the USB configuration attributes
 * @param c Pointer to configuration
 * @param c_attrs Configuration attributes
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_set_config_attrs(usbg_config *c,
				 const struct usbg_config_attrs *c_attrs);

/**
 * @brief Get the USB configuration strings
 * @param c Pointer to configuration
 * @param c_attrs Structure to be filled
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_get_config_attrs(usbg_config *c,
				 struct usbg_config_attrs *c_attrs);

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
 * @param c_strs Structure to be filled
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_get_config_strs(usbg_config *c, int lang,
				struct usbg_config_strs *c_strs);

/**
 * @brief Get the array of languages available in this config
 * @param c Pointer to configuration
 * @param langs array of available language codes
 * @return 0 on success usbg_error if error occurred
 */
extern int usbg_get_config_strs_langs(usbg_config *c, int **langs);

/**
 * @brief Free configuration strings
 * @details This function releases the memory allocated for strings
 *          not for struct usbg_config_strs itself.
 * @param c_strs Strings to be released
 */
static inline void usbg_free_config_strs(struct usbg_config_strs *c_strs)
{
	if (!c_strs)
		return;

	free(c_strs->configuration);
}

/**
 * @brief Set the USB configuration strings
 * @param c Pointer to configuration
 * @param lang USB language ID
 * @param c_strs Configuration strings
 * @return 0 on success, usbg_error on failure.
 */
extern int usbg_set_config_strs(usbg_config *c, int lang,
				const struct usbg_config_strs *c_strs);

/**
 * @brief Set the configuration string
 * @param c Pointer to config
 * @param lang USB language ID
 * @param string Configuration description
 * @return 0 on success, usbg_error on failure.
 */
extern int usbg_set_config_string(usbg_config *c, int lang, const char *string);

/**
 * @brief Add a function to a configuration
 * @param c Pointer to config
 * @param name Name of configuration function binding
 * @param f Pointer to function
 * @return 0 on success, usbg_error on failure.
 */
extern int usbg_add_config_function(usbg_config *c, const char *name,
				    usbg_function *f);

/**
 * @brief Get target function of given binding
 * @param b Binding between configuration and function
 * @return Pointer to USB function which is target for this binding
 */
extern usbg_function *usbg_get_binding_target(usbg_binding *b);

/**
 * @brief Get binding name
 * @param b Pointer to binding
 * @return Binding name or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_binding is valid.
 * For example binding name is valid until someone remove this binding.
 */
extern const char *usbg_get_binding_name(usbg_binding *b);

/**
 * @brief Get binding name into user buffer
 * @param[in] b Pointer to binding
 * @param[out] buf Place where binding name should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family).
 */
extern int usbg_get_binding_name_s(usbg_binding *b, char *buf, int len);

/* USB gadget setup and teardown */

/**
 * @brief Enable a USB gadget device
 * @param g Pointer to gadget
 * @param udc where gadget should be assigned.
 *  If NULL, default one (first) is used.
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_enable_gadget(usbg_gadget *g, usbg_udc *udc);

/**
 * @brief Disable a USB gadget device
 * @param g Pointer to gadget
 * @return 0 on success or usbg_error if error occurred.
 */
extern int usbg_disable_gadget(usbg_gadget *g);

/**
 * @brief Get name of udc
 * @param u Pointer to udc
 * @return UDC name or NULL if error occurred.
 * @warning Returned buffer should not be edited!
 * Returned string is valid as long as passed usbg_state is valid.
 * For example UDC name is valid until usbg_cleanup().
 */
extern const char *usbg_get_udc_name(usbg_udc *u);

/**
 * @brief Get udc name into user buffer
 * @param[in] u Pointer to udc
 * @param[out] buf Place where udc name should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family).
 */
extern int usbg_get_udc_name_s(usbg_udc *u, char *buf, int len);

/**
 * @brief Get udc to which gadget is bound
 * @param g Pointer to gadget
 * @return Pointer to UDC or NULL if gadget is not enabled
 */
extern usbg_udc *usbg_get_gadget_udc(usbg_gadget *g);

/**
 * @brief Get gadget which is attached to this UDC
 * @param u Pointer to udc
 * @return Pointer to gadget or NULL if UDC is free
 */
extern usbg_gadget *usbg_get_udc_gadget(usbg_udc *u);

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
 * @def usbg_for_each_udc(b, c)
 * Iterates over each udc
 */
#define usbg_for_each_udc(u, s)	\
	for (u = usbg_get_first_udc(s); \
	u != NULL; \
	u = usbg_get_next_udc(u))

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
 * @param c Pointer to configuration
 * @return Pointer to binding or NULL if list is empty.
 * @note Bindings are sorted in strings (name) order
 */
extern usbg_binding *usbg_get_first_binding(usbg_config *c);

/**
 * @brief Get first udc in udc list
 * @param s State of library
 * @return Pointer to udc or NULL if list is empty.
 * @note UDCs are sorted in strings (name) order
 */
extern usbg_udc *usbg_get_first_udc(usbg_state *s);

/**
 * @brief Get the next gadget on a list.
 * @param g Pointer to current gadget
 * @return Next gadget or NULL if end of list.
 */
extern usbg_gadget *usbg_get_next_gadget(usbg_gadget *g);

/**
 * @brief Get the next function on a list.
 * @param f Pointer to current function
 * @return Next function or NULL if end of list.
 */
extern usbg_function *usbg_get_next_function(usbg_function *f);

/**
 * @brief Get the next config on a list.
 * @param c Pointer to current config
 * @return Next config or NULL if end of list.
 */
extern usbg_config *usbg_get_next_config(usbg_config *c);

/**
 * @brief Get the next binding on a list.
 * @param b Pointer to current binding
 * @return Next binding or NULL if end of list.
 */
extern usbg_binding *usbg_get_next_binding(usbg_binding *b);

/**
 * @brief Get the next udc on a list.
 * @param u Pointer to current udc
 * @return Next udc or NULL if end of list.
 */
extern usbg_udc *usbg_get_next_udc(usbg_udc *u);

/* Import / Export API */

/**
 * @brief Exports usb function to file
 * @param f Pointer to function to be exported
 * @param stream where function should be saved
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_export_function(usbg_function *f, FILE *stream);

/**
 * @brief Exports configuration to file
 * @param c Pointer to configuration to be exported
 * @param stream where configuration should be saved
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_export_config(usbg_config *c, FILE *stream);

/**
 * @brief Exports whole gadget to file
 * @param g Pointer to gadget to be exported
 * @param stream where gadget should be saved
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_export_gadget(usbg_gadget *g, FILE *stream);

/**
 * @brief Imports usb function from file and adds it to given gadget
 * @param g Gadget where function should be placed
 * @param stream from which function should be imported
 * @param instance name which should be used for new function
 * @param f place for pointer to imported function
 * if NULL this param will be ignored.
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_import_function(usbg_gadget *g, FILE *stream,
				const char *instance, usbg_function **f);

/**
 * @brief Imports usb configuration from file and adds it to given gadget
 * @param g Gadget where configuration should be placed
 * @param stream from which configuration should be imported
 * @param id which should be used for new configuration
 * @param c place for pointer to imported configuration
 * if NULL this param will be ignored.
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_import_config(usbg_gadget *g, FILE *stream, int id,
				usbg_config **c);

/**
 * @brief Imports usb gadget from file
 * @param s current state of library
 * @param stream from which gadget should be imported
 * @param name which should be used for new gadget
 * @param g place for pointer to imported gadget
 * if NULL this param will be ignored.
 * @return 0 on success, usbg_error otherwise
 */
extern int usbg_import_gadget(usbg_state *s, FILE *stream,
			      const char *name, usbg_gadget **g);

/**
 * @brief Get text of error which occurred during last function import
 * @param g gadget where function import error occurred
 * @return Text of error or NULL if no error data
 */
extern const char *usbg_get_func_import_error_text(usbg_gadget *g);

/**
 * @brief Get line number where function import error occurred
 * @param g gadget where function import error occurred
 * @return line number or value below 0 if no error data
 */
extern int usbg_get_func_import_error_line(usbg_gadget *g);

/**
 * @brief Get text of error which occurred during last config import
 * @param g gadget where config import error occurred
 * @return Text of error or NULL if no error data
 */
extern const char *usbg_get_config_import_error_text(usbg_gadget *g);

/**
 * @brief Get line number where config import error occurred
 * @param g gadget where config import error occurred
 * @return line number or value below 0 if no error data
 */
extern int usbg_get_config_import_error_line(usbg_gadget *g);

/**
 * @brief Get text of error which occurred during last gadget import
 * @param s where gadget import error occurred
 * @return Text of error or NULL if no error data
 */
extern const char *usbg_get_gadget_import_error_text(usbg_state *s);

/**
 * @brief Get line number where gadget import error occurred
 * @param s where gadget import error occurred
 * @return line number or value below 0 if no error data
 */
extern int usbg_get_gadget_import_error_line(usbg_state *s);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __USBG_H__ */
