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

static void usbg_parse_function_attrs(struct function *f)
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

static int usbg_parse_functions(char *path, struct gadget *g)
{
	struct function *f;
	int i, n;
	struct dirent **dent;
	char fpath[USBG_MAX_PATH_LENGTH];

	sprintf(fpath, "%s/%s/%s", path, g->name, FUNCTIONS_DIR);

	TAILQ_INIT(&g->functions);

	n = scandir(fpath, &dent, file_select, alphasort);
	for (i=0; i < n; i++) {
		f = malloc(sizeof(struct function));
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

static void usbg_parse_config_attrs(struct config *c)
{
	c->maxpower = usbg_read_dec(c->path, c->name, "MaxPower");
	c->bmattrs = usbg_read_hex(c->path, c->name, "bmAttributes");
	usbg_read_string(c->path, c->name, "strings/0x409/configuration", c->str_cfg);
}

static void usbg_parse_config_bindings(struct config *c)
{
	int i, n;
	struct dirent **dent;
	char bpath[USBG_MAX_PATH_LENGTH];
	struct gadget *g = c->parent;
	struct binding *b;
	struct function *f;

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
				b = malloc(sizeof(struct binding));
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

static int usbg_parse_configs(char *path, struct gadget *g)
{
	struct config *c;
	int i, n;
	struct dirent **dent;
	char cpath[USBG_MAX_PATH_LENGTH];

	sprintf(cpath, "%s/%s/%s", path, g->name, CONFIGS_DIR);

	TAILQ_INIT(&g->configs);

	n = scandir(cpath, &dent, file_select, alphasort);
	for (i=0; i < n; i++) {
		c = malloc(sizeof(struct config));
		c->parent = g;
		strcpy(c->name, dent[i]->d_name);
		strcpy(c->path, cpath);
		usbg_parse_config_attrs(c);
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

static int usbg_parse_gadgets(char *path, struct state *s)
{
	struct gadget *g;
	int i, n;
	struct dirent **dent;

	TAILQ_INIT(&s->gadgets);

	n = scandir(path, &dent, file_select, alphasort);
	for (i=0; i < n; i++) {
		g = malloc(sizeof(struct gadget));
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

static int usbg_init_state(char *path, struct state *s)
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

struct state *usbg_init(char *configfs_path)
{
	int ret;
	struct stat sts;
	char path[USBG_MAX_PATH_LENGTH];
	struct state *s = NULL;

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

	s = malloc(sizeof(struct state));
	if (s)
		usbg_init_state(path, s);
	else
		ERRORNO("couldn't init gadget state\n");

out:
	return s;
}

void usbg_cleanup(struct state *s)
{
	struct gadget *g;
	struct config *c;
	struct binding *b;
	struct function *f;

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

size_t usbg_get_configfs_path_len(struct state *s)
{
	return s ? strlen(s->path) : -1;
}

char *usbg_get_configfs_path(struct state *s, char *buf, size_t len)
{
	return s ? strncpy(buf, s->path, len) : NULL;
}

struct gadget *usbg_get_gadget(struct state *s, const char *name)
{
	struct gadget *g;

	TAILQ_FOREACH(g, &s->gadgets, gnode)
		if (!strcmp(g->name, name))
			return g;

	return NULL;
}

struct function *usbg_get_function(struct gadget *g, const char *name)
{
	struct function *f;

	TAILQ_FOREACH(f, &g->functions, fnode)
		if (!strcmp(f->name, name))
			return f;

	return NULL;
}

struct config *usbg_get_config(struct gadget *g, const char *name)
{
	struct config *c;

	TAILQ_FOREACH(c, &g->configs, cnode)
		if (!strcmp(c->name, name))
			return c;

	return NULL;
}

struct binding *usbg_get_binding(struct config *c, const char *name)
{
	struct binding *b;

	TAILQ_FOREACH(b, &c->bindings, bnode)
		if (!strcmp(b->name, name))
			return b;

	return NULL;
}

struct binding *usbg_get_link_binding(struct config *c, struct function *f)
{
	struct binding *b;

	TAILQ_FOREACH(b, &c->bindings, bnode)
		if (b->target == f)
			return b;

	return NULL;
}

static struct gadget *usbg_create_empty_gadget(struct state *s, char *name)
{
	char gpath[USBG_MAX_PATH_LENGTH];
	struct gadget *g;
	int ret;

	sprintf(gpath, "%s/%s", s->path, name);

	g = malloc(sizeof(struct gadget));
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



struct gadget *usbg_create_gadget_vid_pid(struct state *s, char *name,
		uint16_t idVendor, uint16_t idProduct)
{
	struct gadget *g;

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

struct gadget *usbg_create_gadget(struct state *s, char *name,
		struct gadget_attrs *g_attrs, struct gadget_strs *g_strs)
{
	struct gadget *g;

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

struct gadget_attrs *usbg_get_gadget_attrs(struct gadget *g,
		struct gadget_attrs *g_attrs)
{
	if (g && g_attrs)
		*g_attrs = g->attrs;
	else
		g_attrs = NULL;

	return g_attrs;
}

size_t usbg_get_gadget_name_len(struct gadget *g)
{
	return g ? strlen(g->name) : -1;
}

char *usbg_get_gadget_name(struct gadget *g, char *buf, size_t len)
{
	return g ? strncpy(buf, g->name, len) : NULL;
}

size_t usbg_get_gadget_udc_len(struct gadget *g)
{
	return g ? strlen(g->udc) : -1;
}

char *usbg_get_gadget_udc(struct gadget *g, char *buf, size_t len)
{
	return g ? strncpy(buf, g->udc, len) : NULL;
}

void usbg_set_gadget_attrs(struct gadget *g, struct gadget_attrs *g_attrs)
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

void usbg_set_gadget_vendor_id(struct gadget *g, uint16_t idVendor)
{
	g->attrs.idVendor = idVendor;
	usbg_write_hex16(g->path, g->name, "idVendor", idVendor);
}

void usbg_set_gadget_product_id(struct gadget *g, uint16_t idProduct)
{
	g->attrs.idProduct = idProduct;
	usbg_write_hex16(g->path, g->name, "idProduct", idProduct);
}

void usbg_set_gadget_device_class(struct gadget *g, uint8_t bDeviceClass)
{
	g->attrs.bDeviceClass = bDeviceClass;
	usbg_write_hex8(g->path, g->name, "bDeviceClass", bDeviceClass);
}

void usbg_set_gadget_device_protocol(struct gadget *g, uint8_t bDeviceProtocol)
{
	g->attrs.bDeviceProtocol = bDeviceProtocol;
	usbg_write_hex8(g->path, g->name, "bDeviceProtocol", bDeviceProtocol);
}

void usbg_set_gadget_device_subclass(struct gadget *g, uint8_t bDeviceSubClass)
{
	g->attrs.bDeviceSubClass = bDeviceSubClass;
	usbg_write_hex8(g->path, g->name, "bDeviceSubClass", bDeviceSubClass);
}

void usbg_set_gadget_device_max_packet(struct gadget *g, uint8_t bMaxPacketSize0)
{
	g->attrs.bMaxPacketSize0 = bMaxPacketSize0;
	usbg_write_hex8(g->path, g->name, "bMaxPacketSize0", bMaxPacketSize0);
}

void usbg_set_gadget_device_bcd_device(struct gadget *g, uint16_t bcdDevice)
{
	g->attrs.bcdDevice = bcdDevice;
	usbg_write_hex16(g->path, g->name, "bcdDevice", bcdDevice);
}

void usbg_set_gadget_device_bcd_usb(struct gadget *g, uint16_t bcdUSB)
{
	g->attrs.bcdUSB = bcdUSB;
	usbg_write_hex16(g->path, g->name, "bcdUSB", bcdUSB);
}

struct gadget_strs *usbg_get_gadget_strs(struct gadget *g,
		struct gadget_strs *g_strs)
{
	if (g && g_strs)
		*g_strs = g->strs;
	else
		g_strs = NULL;

	return g_strs;
}

void usbg_set_gadget_strs(struct gadget *g, int lang,
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

void usbg_set_gadget_serial_number(struct gadget *g, int lang, char *serno)
{
	char path[USBG_MAX_PATH_LENGTH];

	sprintf(path, "%s/%s/%s/0x%x", g->path, g->name, STRINGS_DIR, lang);

	mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);

	/* strings in library are hardcoded to US English for now */
	if (lang == LANG_US_ENG)
		strcpy(g->strs.str_ser, serno);

	usbg_write_string(path, "", "serialnumber", serno);
}

void usbg_set_gadget_manufacturer(struct gadget *g, int lang, char *mnf)
{
	char path[USBG_MAX_PATH_LENGTH];

	sprintf(path, "%s/%s/%s/0x%x", g->path, g->name, STRINGS_DIR, lang);

	mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);

	/* strings in library are hardcoded to US English for now */
	if (lang == LANG_US_ENG)
		strcpy(g->strs.str_mnf, mnf);

	usbg_write_string(path, "", "manufacturer", mnf);
}

void usbg_set_gadget_product(struct gadget *g, int lang, char *prd)
{
	char path[USBG_MAX_PATH_LENGTH];

	sprintf(path, "%s/%s/%s/0x%x", g->path, g->name, STRINGS_DIR, lang);

	mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);

	/* strings in library are hardcoded to US English for now */
	if (lang == LANG_US_ENG)
		strcpy(g->strs.str_prd, prd);

	usbg_write_string(path, "", "product", prd);
}

struct function *usbg_create_function(struct gadget *g, enum function_type type, char *instance)
{
	char fpath[USBG_MAX_PATH_LENGTH];
	char name[USBG_MAX_STR_LENGTH];
	struct function *f;
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

	f = malloc(sizeof(struct function));
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

	usbg_parse_function_attrs(f);

	INSERT_TAILQ_STRING_ORDER(&g->functions, fhead, name, f, fnode);

	return f;
}

struct config *usbg_create_config(struct gadget *g, char *name)
{
	char cpath[USBG_MAX_PATH_LENGTH];
	struct config *c;
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

	c = malloc(sizeof(struct config));
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

	usbg_parse_config_attrs(c);

	INSERT_TAILQ_STRING_ORDER(&g->configs, chead, name, c, cnode);

	return c;
}

void usbg_set_config_max_power(struct config *c, int maxpower)
{
	c->maxpower = maxpower;
	usbg_write_dec(c->path, c->name, "MaxPower", maxpower);
}

void usbg_set_config_bm_attrs(struct config *c, int bmattrs)
{
	c->bmattrs = bmattrs;
	usbg_write_hex8(c->path, c->name, "bmAttributes", bmattrs);
}

void usbg_set_config_string(struct config *c, int lang, char *str)
{
	char path[USBG_MAX_PATH_LENGTH];

	sprintf(path, "%s/%s/%s/0x%x", c->path, c->name, STRINGS_DIR, lang);

	mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO);

	/* strings in library are hardcoded to US English for now */
	if (lang == LANG_US_ENG)
		strcpy(c->str_cfg, str);

	usbg_write_string(path, "", "configuration", str);
}

int usbg_add_config_function(struct config *c, char *name, struct function *f)
{
	char bpath[USBG_MAX_PATH_LENGTH];
	char fpath[USBG_MAX_PATH_LENGTH];
	struct binding *b;
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

	b = malloc(sizeof(struct binding));
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

int usbg_get_udcs(struct dirent ***udc_list)
{
	return scandir("/sys/class/udc", udc_list, file_select, alphasort);
}

void usbg_enable_gadget(struct gadget *g, char *udc)
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

void usbg_disable_gadget(struct gadget *g)
{
	strcpy(g->udc, "");
	usbg_write_string(g->path, g->name, "UDC", "");
}

/*
 * USB function-specific attribute configuration
 */

void usbg_set_net_dev_addr(struct function *f, struct ether_addr *dev_addr)
{
	char *str_addr;

	f->attr.net.dev_addr = *dev_addr;

	str_addr = ether_ntoa(dev_addr);
	usbg_write_string(f->path, f->name, "dev_addr", str_addr);
}

void usbg_set_net_host_addr(struct function *f, struct ether_addr *host_addr)
{
	char *str_addr;

	f->attr.net.host_addr = *host_addr;

	str_addr = ether_ntoa(host_addr);
	usbg_write_string(f->path, f->name, "host_addr", str_addr);
}

void usbg_set_net_qmult(struct function *f, int qmult)
{
	f->attr.net.qmult = qmult;
	usbg_write_dec(f->path, f->name, "qmult", qmult);
}

struct gadget *usbg_get_first_gadget(struct state *s)
{
	return s ? TAILQ_FIRST(&s->gadgets) : NULL;
}

struct function *usbg_get_first_function(struct gadget *g)
{
	return g ? TAILQ_FIRST(&g->functions) : NULL;
}

struct config *usbg_get_first_config(struct gadget *g)
{
	return g ? TAILQ_FIRST(&g->configs) : NULL;
}

struct binding *usbg_get_first_binding(struct config *c)
{
	return c ? TAILQ_FIRST(&c->bindings) : NULL;
}

struct gadget *usbg_get_next_gadget(struct gadget *g)
{
	return g ? TAILQ_NEXT(g, gnode) : NULL;
}

struct function *usbg_get_next_function(struct function *f)
{
	return f ? TAILQ_NEXT(f, fnode) : NULL;
}

struct config *usbg_get_next_config(struct config *c)
{
	return c ? TAILQ_NEXT(c, cnode) : NULL;
}

struct binding *usbg_get_next_binding(struct binding *b)
{
	return b ? TAILQ_NEXT(b, bnode) : NULL;
}
