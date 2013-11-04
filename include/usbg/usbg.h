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

#include <dirent.h>
#include <sys/queue.h>
#include <netinet/ether.h>

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
 * @struct gadget
 * @brief USB gadget device attributes
 */
struct gadget
{
	char name[USBG_MAX_NAME_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];
	char udc[USBG_MAX_STR_LENGTH];
	int dclass;
	int dsubclass;
	int dproto;
	int maxpacket;
	int bcddevice;
	int bcdusb;
	int product;
	int vendor;
	char str_ser[USBG_MAX_STR_LENGTH];
	char str_mnf[USBG_MAX_STR_LENGTH];
	char str_prd[USBG_MAX_STR_LENGTH];
	TAILQ_ENTRY(gadget) gnode;
	TAILQ_HEAD(chead, config) configs;
	TAILQ_HEAD(fhead, function) functions;
	struct state *parent;
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
	int maxpower;
	int bmattrs;
	char str_cfg[USBG_MAX_STR_LENGTH];
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
 * @param vendor Gadget vendor ID
 * @param product Gadget product ID
 * @return Pointer to gadget or NULL if the gadget cannot be created
 */
extern struct gadget *usbg_create_gadget(struct state *s, char *name, int vendor, int product);

/**
 * @brief Set the USB gadget device class code
 * @param g Pointer to gadget
 * @param dclass USB device class code
 */
extern void usbg_set_gadget_device_class(struct gadget *g, int dclass);

/**
 * @brief Set the USB gadget protocol code
 * @param g Pointer to gadget
 * @param dprotocol USB protocol code
 */
extern void usbg_set_gadget_device_protocol(struct gadget *g, int dproto);

/**
 * @brief Set the USB gadget device subclass code
 * @param g Pointer to gadget
 * @param dsubclass USB device subclass code
 */
extern void usbg_set_gadget_device_subclass(struct gadget *g, int dsubclass);

/**
 * @brief Set the maximum packet size for a gadget
 * @param g Pointer to gadget
 * @param maxpacket Maximum packet size
 */
extern void usbg_set_gadget_device_max_packet(struct gadget *g, int maxpacket);

/**
 * @brief Set the gadget device BCD release number
 * @param g Pointer to gadget
 * @param bcddevice BCD release number
 */
extern void usbg_set_gadget_device_bcd_device(struct gadget *g, int bcddevice);

/**
 * @brief Set the gadget device BCD USB version
 * @param g Pointer to gadget
 * @param bcdusb BCD USB version
 */
extern void usbg_set_gadget_device_bcd_usb(struct gadget *g, int bcdusb);

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
 * @param maxpower Maximum power (in 2 mA units)
 */
extern void usbg_set_config_max_power(struct config *c, int maxpower);

/**
 * @brief Set the configuration bitmap attributes
 * @param c Pointer to config
 * @param bmattrs Configuration characteristics
 */
extern void usbg_set_config_bm_attrs(struct config *c, int bmattrs);

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
#define usbg_for_each_gadget(g, s)	TAILQ_FOREACH(g, &s->gadgets, gnode)

/**
 * @def usbg_for_each_function(f, g)
 * Iterates over each function
 */
#define usbg_for_each_function(f, g)	TAILQ_FOREACH(f, &g->functions, fnode)

/**
 * @def usbg_for_each_config(c, g)
 * Iterates over each config
 */
#define usbg_for_each_config(c, g)	TAILQ_FOREACH(c, &g->configs, cnode)

/**
 * @def usbg_for_each_binding(b, c)
 * Iterates over each binding
 */
#define usbg_for_each_binding(b, c)	TAILQ_FOREACH(b, &c->bindings, bnode)

/**
 * @}
 */
