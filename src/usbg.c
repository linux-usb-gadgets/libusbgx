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
#include <errno.h>
#include <usbg/usbg.h>
#include <netinet/ether.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define STRINGS_DIR "strings"
#define CONFIGS_DIR "configs"
#define FUNCTIONS_DIR "functions"

/**
 * @file usbg.c
 * @todo Handle buffer overflows
 * @todo Error checking and return code propagation
 */

struct usbg_state
{
	char path[USBG_MAX_PATH_LENGTH];

	TAILQ_HEAD(ghead, usbg_gadget) gadgets;
};

struct usbg_gadget
{
	char name[USBG_MAX_NAME_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];
	char udc[USBG_MAX_STR_LENGTH];

	struct gadget_attrs attrs;
	struct gadget_strs strs;

	TAILQ_ENTRY(usbg_gadget) gnode;
	TAILQ_HEAD(chead, usbg_config) configs;
	TAILQ_HEAD(fhead, usbg_function) functions;
	usbg_state *parent;
};

struct usbg_config
{
	TAILQ_ENTRY(usbg_config) cnode;
	TAILQ_HEAD(bhead, usbg_binding) bindings;
	usbg_gadget *parent;

	char name[USBG_MAX_NAME_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];
	struct config_attrs attrs;
	struct config_strs strs;
};

struct usbg_function
{
	TAILQ_ENTRY(usbg_function) fnode;
	usbg_gadget *parent;

	char name[USBG_MAX_NAME_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];

	enum function_type type;
	union attrs attr;
};

struct usbg_binding
{
	TAILQ_ENTRY(usbg_binding) bnode;
	usbg_config *parent;
	usbg_function *target;

	char name[USBG_MAX_NAME_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];
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

#define ERROR(msg, ...) do {\
                        char *str;\
                        fprintf(stderr, "%s()  "msg" \n", \
                                __func__, ##__VA_ARGS__);\
                        fflush(stderr);\
                    } while (0)

#define ERRORNO(msg, ...) do {\
                        char *str;\
                        fprintf(stderr, "%s()  %s: "msg" \n", \
                                __func__, strerror(errno), ##__VA_ARGS__);\
                        fflush(stderr);\
                    } while (0)

/* Insert in string order */
#define INSERT_TAILQ_STRING_ORDER(HeadPtr, HeadType, NameField, ToInsert, NodeField) \
	do { \
		if (TAILQ_EMPTY(HeadPtr) || \
			(strcmp(ToInsert->NameField, TAILQ_FIRST(HeadPtr)->NameField) < 0)) \
			TAILQ_INSERT_HEAD(HeadPtr, ToInsert, NodeField); \
		else if (strcmp(ToInsert->NameField, TAILQ_LAST(HeadPtr, HeadType)->NameField) > 0) \
			TAILQ_INSERT_TAIL(HeadPtr, ToInsert, NodeField); \
		else { \
			typeof(ToInsert) _cur; \
			TAILQ_FOREACH(_cur, HeadPtr, NodeField) { \
				if (strcmp(ToInsert->NameField, _cur->NameField) > 0) \
					continue; \
				TAILQ_INSERT_BEFORE(_cur, ToInsert, NodeField); \
			} \
		} \
	} while (0)

static int usbg_lookup_function_type(char *name)
{
	int i = 0;
	int max = sizeof(function_names)/sizeof(char *);

	if (!name)
		return -1;

	do {
		if (!strcmp(name, function_names[i]))
			break;
		i++;
	} while (i != max);

	if (i == max)
		i = -1;

	return i;
}

static int bindings_select(const struct dirent *dent)
{
	if (dent->d_type == DT_LNK)
		return 1;
	else
		return 0;
}

static int file_select(const struct dirent *dent)
{
	if ((strcmp(dent->d_name, ".") == 0) || (strcmp(dent->d_name, "..") == 0))
		return 0;
	else
		return 1;
}

static char *usbg_read_buf(char *path, char *name, char *file, char *buf)
{
	char p[USBG_MAX_STR_LENGTH];
	FILE *fp;
	char *ret = NULL;

	sprintf(p, "%s/%s/%s", path, name, file);

	fp = fopen(p, "r");
	if (!fp)
		goto out;

	ret = fgets(buf, USBG_MAX_STR_LENGTH, fp);
	if (ret == NULL) {
		ERROR("read error");
		fclose(fp);
		return ret;
	}

	fclose(fp);

out:
	return ret;
}

static int usbg_read_int(char *path, char *name, char *file, int base)
{
	char buf[USBG_MAX_STR_LENGTH];

	if (usbg_read_buf(path, name, file, buf))
		return strtol(buf, NULL, base);
	else
		return 0;

}

#define usbg_read_dec(p, n, f)	usbg_read_int(p, n, f, 10)
#define usbg_read_hex(p, n, f)	usbg_read_int(p, n, f, 16)

static void usbg_read_string(char *path, char *name, char *file, char *buf)
{
	char *p = NULL;

	p = usbg_read_buf(path, name, file, buf);
	/* Check whether read was successful */
	if (p != NULL) {
		if ((p = strchr(buf, '\n')) != NULL)
				*p = '\0';
	} else {
		/* Set this as empty string */
		*buf = '\0';
	}

}

static void usbg_write_buf(char *path, char *name, char *file, char *buf)
{
	char p[USBG_MAX_STR_LENGTH];
	FILE *fp;
	int err;

	sprintf(p, "%s/%s/%s", path, name, file);

	fp = fopen(p, "w");
	if (!fp) {
		ERRORNO("%s\n", p);
		return;
	}

	fputs(buf, fp);
	fflush(fp);
	err = ferror(fp);
	fclose(fp);
	
	if (err){
		ERROR("write error");
		return;
	}
}

static void usbg_write_int(char *path, char *name, char *file, int value, char *str)
{
	char buf[USBG_MAX_STR_LENGTH];

	sprintf(buf, str, value);
	usbg_write_buf(path, name, file, buf);
}

#define usbg_write_dec(p, n, f, v)	usbg_write_int(p, n, f, v, "%d\n")
#define usbg_write_hex16(p, n, f, v)	usbg_write_int(p, n, f, v, "0x%04x\n")
#define usbg_write_hex8(p, n, f, v)	usbg_write_int(p, n, f, v, "0x%02x\n")

static inline void usbg_write_string(char *path, char *name, char *file, char *buf)
{
	usbg_write_buf(path, name, file, buf);
}

static void usbg_parse_function_attrs(usbg_function *f)
{
	struct ether_addr *addr;
	char str_addr[40];

	switch (f->type) {
	case F_SERIAL:
	case F_ACM:
	case F_OBEX:
		f->attr.serial.port_num = usbg_read_dec(f->path, f->name, "port_num");
		break;
	case F_ECM:
	case F_SUBSET:
	case F_NCM:
	case F_EEM:
	case F_RNDIS:
		usbg_read_string(f->path, f->name, "dev_addr", str_addr);
		addr = ether_aton(str_addr);
		if (addr)
			f->attr.net.dev_addr = *addr;

		usbg_read_string(f->path, f->name, "host_addr", str_addr);
		addr = ether_aton(str_addr);
		if(addr)
			f->attr.net.host_addr = *addr;

		usbg_read_string(f->path, f->name, "ifname", f->attr.net.ifname);
		f->attr.net.qmult = usbg_read_dec(f->path, f->name, "qmult");
		break;
	case F_PHONET:
		usbg_read_string(f->path, f->name, "ifname", f->attr.phonet.ifname);
		break;
	default:
		ERROR("Unsupported function type\n");
	}
}

static int usbg_parse_functions(char *path, usbg_gadget *g)
{
	usbg_function *f;
	int i, n;
	struct dirent **dent;
	char fpath[USBG_MAX_PATH_LENGTH];

	sprintf(fpath, "%s/%s/%s", path, g->name, FUNCTIONS_DIR);

	TAILQ_INIT(&g->functions);

	n = scandir(fpath, &dent, file_select, alphasort);
	for (i=0; i < n; i++) {
		f = malloc(sizeof(usbg_function));
		f->parent = g;
		strcpy(f->name, dent[i]->d_name);
		strcpy(f->path, fpath);
		f->type = usbg_lookup_function_type(strtok(dent[i]->d_name, "."));
		usbg_parse_function_attrs(f);
		TAILQ_INSERT_TAIL(&g->functions, f, fnode);
		free(dent[i]);
	}
	free(dent);

	return 0;
}

static void usbg_parse_config_attrs(char *path, char *name,
		struct config_attrs *c_attrs)
{
	c_attrs->bMaxPower = usbg_read_dec(path, name, "MaxPower");
	c_attrs->bmAttributes = usbg_read_hex(path, name, "bmAttributes");
}

static void usbg_parse_config_strs(char *path, char *name,
		struct config_strs *c_attrs)
{
	/* Hardcoded to US English right now*/
	int lang = LANG_US_ENG;
	char spath[USBG_MAX_PATH_LENGTH];

	sprintf(spath, "%s/%s/%s/0x%x", path, name, STRINGS_DIR, lang);
	usbg_read_string(spath, "", "configuration", c_attrs->configuration);
}

static void usbg_parse_config_bindings(usbg_config *c)
{
	int i, n;
	struct dirent **dent;
	char bpath[USBG_MAX_PATH_LENGTH];
	usbg_gadget *g = c->parent;
	usbg_binding *b;
	usbg_function *f;

	sprintf(bpath, "%s/%s", c->path, c->name);

	TAILQ_INIT(&c->bindings);

	n = scandir(bpath, &dent, bindings_select, alphasort);
	for (i=0; i < n; i++) {
		TAILQ_FOREACH(f, &g->functions, fnode) {
			int n;
			char contents[USBG_MAX_STR_LENGTH];
			char cpath[USBG_MAX_PATH_LENGTH];
			char fname[40];

			sprintf(cpath, "%s/%s", bpath, dent[i]->d_name);
			n = readlink(cpath, contents, USBG_MAX_PATH_LENGTH);
			if (n<0)
				ERRORNO("bytes %d contents %s\n", n, contents);
			strcpy(fname, f->name);
			if (strstr(contents, strtok(fname, "."))) {
				b = malloc(sizeof(usbg_binding));
				strcpy(b->name, dent[i]->d_name);
				strcpy(b->path, bpath);
				b->target = f;
				TAILQ_INSERT_TAIL(&c->bindings, b, bnode);
				break;
			}
		}
		free(dent[i]);
	}
	free(dent);
}

static int usbg_parse_configs(char *path, usbg_gadget *g)
{
	usbg_config *c;
	int i, n;
	struct dirent **dent;
	char cpath[USBG_MAX_PATH_LENGTH];

	sprintf(cpath, "%s/%s/%s", path, g->name, CONFIGS_DIR);

	TAILQ_INIT(&g->configs);

	n = scandir(cpath, &dent, file_select, alphasort);
	for (i=0; i < n; i++) {
		c = malloc(sizeof(usbg_config));
		c->parent = g;
		strcpy(c->name, dent[i]->d_name);
		strcpy(c->path, cpath);
		usbg_parse_config_attrs(cpath, c->name, &c->attrs);
		usbg_parse_config_strs(cpath, c->name, &c->strs);
		usbg_parse_config_bindings(c);
		TAILQ_INSERT_TAIL(&g->configs, c, cnode);
		free(dent[i]);
	}
	free(dent);

	return 0;
}

static void usbg_parse_gadget_attrs(char *path, char *name,
		struct gadget_attrs *g_attrs)
{
	/* Actual attributes */
	g_attrs->bcdUSB = (uint16_t)usbg_read_hex(path, name, "bcdUSB");
	g_attrs->bDeviceClass = (uint8_t)usbg_read_hex(path, name, "bDeviceClass");
	g_attrs->bDeviceSubClass = (uint8_t)usbg_read_hex(path, name, "bDeviceSubClass");
	g_attrs->bDeviceProtocol = (uint8_t)usbg_read_hex(path, name, "bDeviceProtocol");
	g_attrs->bMaxPacketSize0 = (uint8_t)usbg_read_hex(path, name, "bMaxPacketSize0");
	g_attrs->idVendor = (uint16_t)usbg_read_hex(path, name, "idVendor");
	g_attrs->idProduct = (uint16_t)usbg_read_hex(path, name, "idProduct");
	g_attrs->bcdDevice = (uint16_t)usbg_read_hex(path, name, "bcdDevice");
}

static void usbg_parse_strings(char *path, char *name, struct gadget_strs *g_strs)
{
	/* Strings - hardcoded to U.S. English only for now */
	int lang = LANG_US_ENG;
	char spath[USBG_MAX_PATH_LENGTH];

	sprintf(spath, "%s/%s/%s/0x%x", path, name, STRINGS_DIR, lang);

	usbg_read_string(spath, "", "serialnumber", g_strs->str_ser);
	usbg_read_string(spath, "", "manufacturer", g_strs->str_mnf);
	usbg_read_string(spath, "", "product", g_strs->str_prd);
}

static int usbg_parse_gadgets(char *path, usbg_state *s)
{
	usbg_gadget *g;
	int i, n;
	struct dirent **dent;

	TAILQ_INIT(&s->gadgets);

	n = scandir(path, &dent, file_select, alphasort);
	for (i=0; i < n; i++) {
		g = malloc(sizeof(usbg_gadget));
		strcpy(g->name, dent[i]->d_name);
		strcpy(g->path, s->path);
		g->parent = s;
		/* UDC bound to, if any */
		usbg_read_string(path, g->name, "UDC", g->udc);
		usbg_parse_gadget_attrs(path, g->name, &g->attrs);
		usbg_parse_strings(path, g->name, &g->strs);
		usbg_parse_functions(path, g);
		usbg_parse_configs(path, g);
		TAILQ_INSERT_TAIL(&s->gadgets, g, gnode);
		free(dent[i]);
	}
	free(dent);

	return 0;
}

static int usbg_init_state(char *path, usbg_state *s)
{
	strcpy(s->path, path);

	if (usbg_parse_gadgets(path, s) < 0) {
		ERRORNO("unable to parse %s\n", path);
		return -1;
	}

	return 0;
}

/*
 * User API
 */

usbg_state *usbg_init(char *configfs_path)
{
	int ret;
	struct stat sts;
	char path[USBG_MAX_PATH_LENGTH];
	usbg_state *s = NULL;

	strcpy(path, configfs_path);
	ret = stat(strcat(path, "/usb_gadget"), &sts);
	if (ret < 0) {
		ERRORNO("%s", path);
		goto out;
	}

	if (!S_ISDIR(sts.st_mode)) {
		ERRORNO("%s", path);
		goto out;
	}

	s = malloc(sizeof(usbg_state));
	if (s)
		usbg_init_state(path, s);
	else
		ERRORNO("couldn't init gadget state\n");

out:
	return s;
}

void usbg_cleanup(usbg_state *s)
{
	usbg_gadget *g;
	usbg_config *c;
	usbg_binding *b;
	usbg_function *f;

	while (!TAILQ_EMPTY(&s->gadgets)) {
		g = TAILQ_FIRST(&s->gadgets);
		while (!TAILQ_EMPTY(&g->configs)) {
			c = TAILQ_FIRST(&g->configs);
			while(!TAILQ_EMPTY(&c->bindings)) {
				b = TAILQ_FIRST(&c->bindings);
				TAILQ_REMOVE(&c->bindings, b, bnode);
				free(b);
			}
			TAILQ_REMOVE(&g->configs, c, cnode);
			free(c);
		}
		while (!TAILQ_EMPTY(&g->functions)) {
			f = TAILQ_FIRST(&g->functions);
			TAILQ_REMOVE(&g->functions, f, fnode);
			free(f);
		}
		TAILQ_REMOVE(&s->gadgets, g, gnode);
		free(g);
	}

	free(s);
}

size_t usbg_get_configfs_path_len(usbg_state *s)
{
	return s ? strlen(s->path) : -1;
}

char *usbg_get_configfs_path(usbg_state *s, char *buf, size_t len)
{
	return s ? strncpy(buf, s->path, len) : NULL;
}

usbg_gadget *usbg_get_gadget(usbg_state *s, const char *name)
{
	usbg_gadget *g;

	TAILQ_FOREACH(g, &s->gadgets, gnode)
		if (!strcmp(g->name, name))
			return g;

	return NULL;
}

usbg_function *usbg_get_function(usbg_gadget *g, const char *name)
{
	usbg_function *f;

	TAILQ_FOREACH(f, &g->functions, fnode)
		if (!strcmp(f->name, name))
			return f;

	return NULL;
}

usbg_config *usbg_get_config(usbg_gadget *g, const char *name)
{
	usbg_config *c;

	TAILQ_FOREACH(c, &g->configs, cnode)
		if (!strcmp(c->name, name))
			return c;

	return NULL;
}

usbg_binding *usbg_get_binding(usbg_config *c, const char *name)
{
	usbg_binding *b;

	TAILQ_FOREACH(b, &c->bindings, bnode)
		if (!strcmp(b->name, name))
			return b;

	return NULL;
}

usbg_binding *usbg_get_link_binding(usbg_config *c, usbg_function *f)
{
	usbg_binding *b;

	TAILQ_FOREACH(b, &c->bindings, bnode)
		if (b->target == f)
			return b;

	return NULL;
}

static usbg_gadget *usbg_create_empty_gadget(usbg_state *s, char *name)
{
	char gpath[USBG_MAX_PATH_LENGTH];
	usbg_gadget *g;
	int ret;

	sprintf(gpath, "%s/%s", s->path, name);

	g = malloc(sizeof(usbg_gadget));
	if (!g) {
		ERRORNO("allocating gadget\n");
		return NULL;
	}

	TAILQ_INIT(&g->configs);
	TAILQ_INIT(&g->functions);
	strcpy(g->name, name);
	strcpy(g->path, s->path);
	g->parent = s;

	ret = mkdir(gpath, S_IRWXU|S_IRWXG|S_IRWXO);
	if (ret < 0) {
		ERRORNO("%s\n", gpath);
		free(g);
		return NULL;
	}

	/* Should be empty but read the default */
	usbg_read_string(g->path, g->name, "UDC", g->udc);

	return g;
}



usbg_gadget *usbg_create_gadget_vid_pid(usbg_state *s, char *name,
		uint16_t idVendor, uint16_t idProduct)
{
	usbg_gadget *g;

	if (!s)
		return NULL;

	g = usbg_get_gadget(s, name);
	if (g) {
		ERROR("duplicate gadget name\n");
		return NULL;
	}

	g = usbg_create_empty_gadget(s, name);

	/* Check if gadget creation was successful and set attributes */
	if (g) {
		usbg_write_hex16(s->path, name, "idVendor", idVendor);
		usbg_write_hex16(s->path, name, "idProduct", idProduct);

		usbg_parse_gadget_attrs(s->path, name, &g->attrs);
		usbg_parse_strings(s->path, name, &g->strs);

		INSERT_TAILQ_STRING_ORDER(&s->gadgets, ghead, name, g, gnode);
	}

	return g;
}

usbg_gadget *usbg_create_gadget(usbg_state *s, char *name,
		struct gadget_attrs *g_attrs, struct gadget_strs *g_strs)
{
	usbg_gadget *g;

	if (!s)
		return NULL;

	g = usbg_get_gadget(s, name);
	if (g) {
		ERROR("duplicate gadget name\n");
		return NULL;
	}

	g = usbg_create_empty_gadget(s, name);

	/* Check if gadget creation was successful and set attrs and strings */
	if (g) {
		if (g_attrs)
			usbg_set_gadget_attrs(g, g_attrs);
		else
			usbg_parse_gadget_attrs(s->path, name, &g->attrs);

		if (g_strs)
			usbg_set_gadget_strs(g, LANG_US_ENG, g_strs);
		else
			usbg_parse_strings(s->path, name, &g->strs);

		INSERT_TAILQ_STRING_ORDER(&s->gadgets, ghead, name, g, gnode);
	}

	return g;
}

struct gadget_attrs *usbg_get_gadget_attrs(usbg_gadget *g,
		struct gadget_attrs *g_attrs)
{
	if (g && g_attrs)
		*g_attrs = g->attrs;
	else
		g_attrs = NULL;

	return g_attrs;
}

size_t usbg_get_gadget_name_len(usbg_gadget *g)
{
	return g ? strlen(g->name) : -1;
}

char *usbg_get_gadget_name(usbg_gadget *g, char *buf, size_t len)
{
	return g ? strncpy(buf, g->name, len) : NULL;
}

size_t usbg_get_gadget_udc_len(usbg_gadget *g)
{
	return g ? strlen(g->udc) : -1;
}

char *usbg_get_gadget_udc(usbg_gadget *g, char *buf, size_t len)
{
	return g ? strncpy(buf, g->udc, len) : NULL;
}

void usbg_set_gadget_attrs(usbg_gadget *g, struct gadget_attrs *g_attrs)
{
	if (!g || !g_attrs)
		return;

	g->attrs = *g_attrs;
	usbg_write_hex16(g->path, g->name, "bcdUSB", g_attrs->bcdUSB);
	usbg_write_hex8(g->path, g->name, "bDeviceClass", g_attrs->bDeviceClass);
	usbg_write_hex8(g->path, g->name, "bDeviceSubClass", g_attrs->bDeviceSubClass);
	usbg_write_hex8(g->path, g->name, "bDeviceProtocol", g_attrs->bDeviceProtocol);
	usbg_write_hex8(g->path, g->name, "bMaxPacketSize0", g_attrs->bMaxPacketSize0);
	usbg_write_hex16(g->path, g->name, "idVendor", g_attrs->idVendor);
	usbg_write_hex16(g->path, g->name, "idProduct", g_attrs->idProduct);
	usbg_write_hex16(g->path, g->name, "bcdDevice", g_attrs->bcdDevice);
}

void usbg_set_gadget_vendor_id(usbg_gadget *g, uint16_t idVendor)
{
	g->attrs.idVendor = idVendor;
	usbg_write_hex16(g->path, g->name, "idVendor", idVendor);
}

void usbg_set_gadget_product_id(usbg_gadget *g, uint16_t idProduct)
{
	g->attrs.idProduct = idProduct;
	usbg_write_hex16(g->path, g->name, "idProduct", idProduct);
}

void usbg_set_gadget_device_class(usbg_gadget *g, uint8_t bDeviceClass)
{
	g->attrs.bDeviceClass = bDeviceClass;
	usbg_write_hex8(g->path, g->name, "bDeviceClass", bDeviceClass);
}

void usbg_set_gadget_device_protocol(usbg_gadget *g, uint8_t bDeviceProtocol)
{
	g->attrs.bDeviceProtocol = bDeviceProtocol;
	usbg_write_hex8(g->path, g->name, "bDeviceProtocol", bDeviceProtocol);
}

void usbg_set_gadget_device_subclass(usbg_gadget *g, uint8_t bDeviceSubClass)
{
	g->attrs.bDeviceSubClass = bDeviceSubClass;
	usbg_write_hex8(g->path, g->name, "bDeviceSubClass", bDeviceSubClass);
}

void usbg_set_gadget_device_max_packet(usbg_gadget *g, uint8_t bMaxPacketSize0)
{
	g->attrs.bMaxPacketSize0 = bMaxPacketSize0;
	usbg_write_hex8(g->path, g->name, "bMaxPacketSize0", bMaxPacketSize0);
}

void usbg_set_gadget_device_bcd_device(usbg_gadget *g, uint16_t bcdDevice)
{
	g->attrs.bcdDevice = bcdDevice;
	usbg_write_hex16(g->path, g->name, "bcdDevice", bcdDevice);
}

void usbg_set_gadget_device_bcd_usb(usbg_gadget *g, uint16_t bcdUSB)
{
	g->attrs.bcdUSB = bcdUSB;
	usbg_write_hex16(g->path, g->name, "bcdUSB", bcdUSB);
}

struct gadget_strs *usbg_get_gadget_strs(usbg_gadget *g,
		struct gadget_strs *g_strs)
{
	if (g && g_strs)
		*g_strs = g->strs;
	else
		g_strs = NULL;

	return g_strs;
}

void usbg_set_gadget_strs(usbg_gadget *g, int lang,
		struct gadget_strs *g_strs)
{
	char path[USBG_MAX_PATH_LENGTH];

	sprintf(path, "%s/%s/%s/0x%x", g->path, g->name, STRINGS_DIR, lang);

	mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);

	/* strings in library are hardcoded to US English for now */
	if (lang == LANG_US_ENG)
		g->strs = *g_strs;

	usbg_write_string(path, "", "serialnumber", g_strs->str_ser);
	usbg_write_string(path, "", "manufacturer", g_strs->str_mnf);
	usbg_write_string(path, "", "product", g_strs->str_prd);
}

void usbg_set_gadget_serial_number(usbg_gadget *g, int lang, char *serno)
{
	char path[USBG_MAX_PATH_LENGTH];

	sprintf(path, "%s/%s/%s/0x%x", g->path, g->name, STRINGS_DIR, lang);

	mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);

	/* strings in library are hardcoded to US English for now */
	if (lang == LANG_US_ENG)
		strcpy(g->strs.str_ser, serno);

	usbg_write_string(path, "", "serialnumber", serno);
}

void usbg_set_gadget_manufacturer(usbg_gadget *g, int lang, char *mnf)
{
	char path[USBG_MAX_PATH_LENGTH];

	sprintf(path, "%s/%s/%s/0x%x", g->path, g->name, STRINGS_DIR, lang);

	mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);

	/* strings in library are hardcoded to US English for now */
	if (lang == LANG_US_ENG)
		strcpy(g->strs.str_mnf, mnf);

	usbg_write_string(path, "", "manufacturer", mnf);
}

void usbg_set_gadget_product(usbg_gadget *g, int lang, char *prd)
{
	char path[USBG_MAX_PATH_LENGTH];

	sprintf(path, "%s/%s/%s/0x%x", g->path, g->name, STRINGS_DIR, lang);

	mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);

	/* strings in library are hardcoded to US English for now */
	if (lang == LANG_US_ENG)
		strcpy(g->strs.str_prd, prd);

	usbg_write_string(path, "", "product", prd);
}

usbg_function *usbg_create_function(usbg_gadget *g, enum function_type type,
		char *instance, union attrs *f_attrs)
{
	char fpath[USBG_MAX_PATH_LENGTH];
	char name[USBG_MAX_STR_LENGTH];
	usbg_function *f;
	int ret;

	if (!g)
		return NULL;

	/**
	 * @todo Check for legal function type
	 */
	sprintf(name, "%s.%s", function_names[type], instance);
	f = usbg_get_function(g, name);
	if (f) {
		ERROR("duplicate function name\n");
		return NULL;
	}

	sprintf(fpath, "%s/%s/%s/%s", g->path, g->name, FUNCTIONS_DIR, name);

	f = malloc(sizeof(usbg_function));
	if (!f) {
		ERRORNO("allocating function\n");
		return NULL;
	}

	strcpy(f->name, name);
	sprintf(f->path, "%s/%s/%s", g->path, g->name, FUNCTIONS_DIR);
	f->type = type;

	ret = mkdir(fpath, S_IRWXU|S_IRWXG|S_IRWXO);
	if (ret < 0) {
		ERRORNO("%s\n", fpath);
		free(f);
		return NULL;
	}

	if (f_attrs)
		usbg_set_function_attrs(f, f_attrs);
	else
		usbg_parse_function_attrs(f);

	INSERT_TAILQ_STRING_ORDER(&g->functions, fhead, name, f, fnode);

	return f;
}

usbg_config *usbg_create_config(usbg_gadget *g, char *name,
		struct config_attrs *c_attrs, struct config_strs *c_strs)
{
	char cpath[USBG_MAX_PATH_LENGTH];
	usbg_config *c;
	int ret;

	if (!g)
		return NULL;

	/**
	 * @todo Check for legal configuration name
	 */
	c = usbg_get_config(g, name);
	if (c) {
		ERROR("duplicate configuration name\n");
		return NULL;
	}

	sprintf(cpath, "%s/%s/%s/%s", g->path, g->name, CONFIGS_DIR, name);

	c = malloc(sizeof(usbg_config));
	if (!c) {
		ERRORNO("allocating configuration\n");
		return NULL;
	}

	TAILQ_INIT(&c->bindings);
	strcpy(c->name, name);
	sprintf(c->path, "%s/%s/%s", g->path, g->name, CONFIGS_DIR);

	ret = mkdir(cpath, S_IRWXU|S_IRWXG|S_IRWXO);
	if (ret < 0) {
		ERRORNO("%s\n", cpath);
		free(c);
		return NULL;
	}

	if (c_attrs)
		usbg_set_config_attrs(c, c_attrs);
	else
		usbg_parse_config_attrs(c->path, c->name, &c->attrs);

	if (c_strs)
		usbg_set_config_string(c, LANG_US_ENG, c_strs->configuration);
	else
		usbg_parse_config_strs(c->path, c->name, &c->strs);

	INSERT_TAILQ_STRING_ORDER(&g->configs, chead, name, c, cnode);

	return c;
}

size_t usbg_get_config_name_len(usbg_config *c)
{
	return c ? strlen(c->name) : -1;
}

char *usbg_get_config_name(usbg_config *c, char *buf, size_t len)
{
	return c ? strncpy(buf, c->name, len) : NULL;
}

size_t usbg_get_function_name_len(usbg_function *f)
{
	return f ? strlen(f->name) : -1;
}

char *usbg_get_function_name(usbg_function *f, char *buf, size_t len)
{
	return f ? strncpy(buf, f->name, len) : NULL;
}

void usbg_set_config_attrs(usbg_config *c, struct config_attrs *c_attrs)
{
	if (!c || !c_attrs)
		return;

	c->attrs = *c_attrs;

	usbg_write_dec(c->path, c->name, "MaxPower", c_attrs->bMaxPower);
	usbg_write_hex8(c->path, c->name, "bmAttributes", c_attrs->bmAttributes);
}

struct config_attrs *usbg_get_config_attrs(usbg_config *c,
		struct config_attrs *c_attrs)
{
	if (c && c_attrs)
		*c_attrs = c->attrs;
	else
		c_attrs = NULL;

	return c_attrs;
}

void usbg_set_config_max_power(usbg_config *c, int bMaxPower)
{
	c->attrs.bMaxPower = bMaxPower;
	usbg_write_dec(c->path, c->name, "MaxPower", bMaxPower);
}

void usbg_set_config_bm_attrs(usbg_config *c, int bmAttributes)
{
	c->attrs.bmAttributes = bmAttributes;
	usbg_write_hex8(c->path, c->name, "bmAttributes", bmAttributes);
}

struct config_strs *usbg_get_config_strs(usbg_config *c,
		struct config_strs *c_strs)
{
	if (c && c_strs)
		*c_strs = c->strs;
	else
		c_strs = NULL;

	return c_strs;
}

void usbg_set_config_strs(usbg_config *c, int lang,
		struct config_strs *c_strs)
{
	usbg_set_config_string(c, lang, c_strs->configuration);
}

void usbg_set_config_string(usbg_config *c, int lang, char *str)
{
	char path[USBG_MAX_PATH_LENGTH];

	sprintf(path, "%s/%s/%s/0x%x", c->path, c->name, STRINGS_DIR, lang);

	mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);

	/* strings in library are hardcoded to US English for now */
	if (lang == LANG_US_ENG)
		strcpy(c->strs.configuration, str);

	usbg_write_string(path, "", "configuration", str);
}

int usbg_add_config_function(usbg_config *c, char *name, usbg_function *f)
{
	char bpath[USBG_MAX_PATH_LENGTH];
	char fpath[USBG_MAX_PATH_LENGTH];
	usbg_binding *b;
	int ret = -1;

	if (!c || !f)
		return ret;

	b = usbg_get_binding(c, name);
	if (b) {
		ERROR("duplicate binding name\n");
		return ret;
	}

	b = usbg_get_link_binding(c, f);
	if (b) {
		ERROR("duplicate binding link\n");
		return ret;
	}

	sprintf(bpath, "%s/%s/%s", c->path, c->name, name);
	sprintf(fpath, "%s/%s", f->path, f->name);

	b = malloc(sizeof(usbg_binding));
	if (!b) {
		ERRORNO("allocating binding\n");
		return -1;
	}

	ret = symlink(fpath, bpath);
	if (ret < 0) {
		ERRORNO("%s -> %s\n", bpath, fpath);
		return ret;
	}

	strcpy(b->name, name);
	strcpy(b->path, bpath);
	b->target = f;
	b->parent = c;

	INSERT_TAILQ_STRING_ORDER(&c->bindings, bhead, name, b, bnode);

	return 0;
}

usbg_function *usbg_get_binding_target(usbg_binding *b)
{
	return b ? b->target : NULL;
}

size_t usbg_get_binding_name_len(usbg_binding *b)
{
	return b ? strlen(b->name) : -1;
}

char *usbg_get_binding_name(usbg_binding *b, char *buf, size_t len)
{
	return b ? strncpy(buf, b->name, len) : NULL;
}

int usbg_get_udcs(struct dirent ***udc_list)
{
	return scandir("/sys/class/udc", udc_list, file_select, alphasort);
}

void usbg_enable_gadget(usbg_gadget *g, char *udc)
{
	char gudc[USBG_MAX_STR_LENGTH];
	struct dirent **udc_list;
	int n;

	if (!udc) {
		n = usbg_get_udcs(&udc_list);
		if (!n)
			return;
		strcpy(gudc, udc_list[0]->d_name);
		while (n--)
			free(udc_list[n]);
		free(udc_list);
	} else
		strcpy (gudc, udc);

	strcpy(g->udc, gudc);
	usbg_write_string(g->path, g->name, "UDC", gudc);
}

void usbg_disable_gadget(usbg_gadget *g)
{
	strcpy(g->udc, "");
	usbg_write_string(g->path, g->name, "UDC", "");
}

/*
 * USB function-specific attribute configuration
 */

enum function_type usbg_get_function_type(usbg_function *f)
{
	return f->type;
}

union attrs *usbg_get_function_attrs(usbg_function *f, union attrs *f_attrs)
{
	if (f && f_attrs)
		*f_attrs = f->attr;
	else
		f_attrs = NULL;

	return f_attrs;
}

void usbg_set_function_attrs(usbg_function *f, union attrs *f_attrs)
{
	char *addr;

	if (!f || !f_attrs)
		return;

	f->attr = *f_attrs;

	switch (f->type) {
	case F_SERIAL:
	case F_ACM:
	case F_OBEX:
		usbg_write_dec(f->path, f->name, "port_num", f_attrs->serial.port_num);
		break;
	case F_ECM:
	case F_SUBSET:
	case F_NCM:
	case F_EEM:
	case F_RNDIS:
		addr = ether_ntoa(&f_attrs->net.dev_addr);
		usbg_write_string(f->path, f->name, "dev_addr", addr);

		addr = ether_ntoa(&f_attrs->net.host_addr);
		usbg_write_string(f->path, f->name, "host_addr", addr);

		usbg_write_string(f->path, f->name, "ifname", f_attrs->net.ifname);

		usbg_write_dec(f->path, f->name, "qmult", f_attrs->net.qmult);
		break;
	case F_PHONET:
		usbg_write_string(f->path, f->name, "ifname", f_attrs->phonet.ifname);
		break;
	default:
		ERROR("Unsupported function type\n");
	}
}

void usbg_set_net_dev_addr(usbg_function *f, struct ether_addr *dev_addr)
{
	char *str_addr;

	f->attr.net.dev_addr = *dev_addr;

	str_addr = ether_ntoa(dev_addr);
	usbg_write_string(f->path, f->name, "dev_addr", str_addr);
}

void usbg_set_net_host_addr(usbg_function *f, struct ether_addr *host_addr)
{
	char *str_addr;

	f->attr.net.host_addr = *host_addr;

	str_addr = ether_ntoa(host_addr);
	usbg_write_string(f->path, f->name, "host_addr", str_addr);
}

void usbg_set_net_qmult(usbg_function *f, int qmult)
{
	f->attr.net.qmult = qmult;
	usbg_write_dec(f->path, f->name, "qmult", qmult);
}

usbg_gadget *usbg_get_first_gadget(usbg_state *s)
{
	return s ? TAILQ_FIRST(&s->gadgets) : NULL;
}

usbg_function *usbg_get_first_function(usbg_gadget *g)
{
	return g ? TAILQ_FIRST(&g->functions) : NULL;
}

usbg_config *usbg_get_first_config(usbg_gadget *g)
{
	return g ? TAILQ_FIRST(&g->configs) : NULL;
}

usbg_binding *usbg_get_first_binding(usbg_config *c)
{
	return c ? TAILQ_FIRST(&c->bindings) : NULL;
}

usbg_gadget *usbg_get_next_gadget(usbg_gadget *g)
{
	return g ? TAILQ_NEXT(g, gnode) : NULL;
}

usbg_function *usbg_get_next_function(usbg_function *f)
{
	return f ? TAILQ_NEXT(f, fnode) : NULL;
}

usbg_config *usbg_get_next_config(usbg_config *c)
{
	return c ? TAILQ_NEXT(c, cnode) : NULL;
}

usbg_binding *usbg_get_next_binding(usbg_binding *b)
{
	return b ? TAILQ_NEXT(b, bnode) : NULL;
}
