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

#include <netinet/ether.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include "usbg/usbg_internal.h"

/**
 * @file usbg.c
 */

extern struct usbg_function_type usbg_f_type_acm;
extern struct usbg_function_type usbg_f_type_serial;
extern struct usbg_function_type usbg_f_type_obex;
extern struct usbg_function_type usbg_f_type_ecm;
extern struct usbg_function_type usbg_f_type_subset;
extern struct usbg_function_type usbg_f_type_ncm;
extern struct usbg_function_type usbg_f_type_eem;
extern struct usbg_function_type usbg_f_type_rndis;
extern struct usbg_function_type usbg_f_type_ffs;
extern struct usbg_function_type usbg_f_type_midi;
extern struct usbg_function_type usbg_f_type_ms;
extern struct usbg_function_type usbg_f_type_phonet;
extern struct usbg_function_type usbg_f_type_loopback;

/**
 * @var function_types
 * @brief Types of functions supported by library
 */
struct usbg_function_type* function_types[] = {
	[F_ACM] = &usbg_f_type_acm,
	[F_SERIAL] = &usbg_f_type_serial,
	[F_OBEX] = &usbg_f_type_obex,
	[F_ECM] = &usbg_f_type_ecm,
	[F_SUBSET] = &usbg_f_type_subset,
	[F_NCM] = &usbg_f_type_ncm,
	[F_EEM] = &usbg_f_type_eem,
	[F_RNDIS] = &usbg_f_type_rndis,
	[F_FFS] = &usbg_f_type_ffs,
	[F_MIDI] = &usbg_f_type_midi,
	[F_MASS_STORAGE] = &usbg_f_type_ms,
	[F_PHONET] = &usbg_f_type_phonet,
	[F_LOOPBACK] = &usbg_f_type_loopback,
};

ARRAY_SIZE_SENTINEL(function_types, USBG_FUNCTION_TYPE_MAX);

const char *gadget_attr_names[] =
{
	"bcdUSB",
	"bDeviceClass",
	"bDeviceSubClass",
	"bDeviceProtocol",
	"bMaxPacketSize0",
	"idVendor",
	"idProduct",
	"bcdDevice"
};

ARRAY_SIZE_SENTINEL(gadget_attr_names, USBG_GADGET_ATTR_MAX);

const char *gadget_str_names[] =
{
	"product",
	"manufacturer",
	"serialnumber",
};

ARRAY_SIZE_SENTINEL(gadget_str_names, USBG_GADGET_STR_MAX);

int usbg_lookup_function_attrs_type(int f_type)
{
	int ret;

	switch (f_type) {
	case F_SERIAL:
	case F_ACM:
	case F_OBEX:
		ret = USBG_F_ATTRS_SERIAL;
		break;
	case F_ECM:
	case F_SUBSET:
	case F_NCM:
	case F_EEM:
	case F_RNDIS:
		ret = USBG_F_ATTRS_NET;
		break;
	case F_PHONET:
		ret = USBG_F_ATTRS_PHONET;
		break;
	case F_FFS:
		ret = USBG_F_ATTRS_FFS;
		break;
	case F_MASS_STORAGE:
		ret = USBG_F_ATTRS_MS;
		break;
	case F_MIDI:
		ret = USBG_F_ATTRS_MIDI;
		break;
	case F_LOOPBACK:
		ret = USBG_F_ATTRS_LOOPBACK;
		break;
	default:
		ret = USBG_ERROR_NOT_SUPPORTED;
	}

	return ret;
}

int usbg_lookup_function_type(const char *name)
{
	int i = USBG_FUNCTION_TYPE_MIN;

	if (!name)
		return USBG_ERROR_INVALID_PARAM;

	do {
		if (!strcmp(name, function_types[i]->name))
			return i;
		i++;
	} while (i != USBG_FUNCTION_TYPE_MAX);

	return USBG_ERROR_NOT_FOUND;
}

const char *usbg_get_function_type_str(usbg_function_type type)
{
	return type >= USBG_FUNCTION_TYPE_MIN &&
		type < USBG_FUNCTION_TYPE_MAX ?
		function_types[type]->name : NULL;
}

int usbg_lookup_gadget_attr(const char *name)
{
	int i = USBG_GADGET_ATTR_MIN;

	if (!name)
		return USBG_ERROR_INVALID_PARAM;

	do {
		if (!strcmp(name, gadget_attr_names[i]))
			return i;
		i++;
	} while (i != USBG_GADGET_ATTR_MAX);

	return USBG_ERROR_NOT_FOUND;
}

int usbg_lookup_gadget_str(const char *name)
{
	int i = USBG_GADGET_STR_MIN;

	if (!name)
		return USBG_ERROR_INVALID_PARAM;

	do {
		if (!strcmp(name, gadget_str_names[i]))
			return i;
		i++;
	} while (i != USBG_GADGET_STR_MAX);

	return USBG_ERROR_NOT_FOUND;
}

const char *usbg_get_gadget_attr_str(usbg_gadget_attr attr)
{
	return attr >= USBG_GADGET_ATTR_MIN &&
		attr < USBG_GADGET_ATTR_MAX ?
		gadget_attr_names[attr] : NULL;
}

const char *usbg_get_gadget_str_name(usbg_gadget_str str)
{
	return str >= USBG_GADGET_STR_MIN &&
		str < USBG_GADGET_STR_MAX ?
		gadget_str_names[str] : NULL;
}

static int usbg_split_function_instance_type(const char *full_name,
		usbg_function_type *f_type, const char **instance)
{
	const char *dot;
	char *type_name = NULL;
	int f_type_ret;
	usbg_error ret = USBG_ERROR_INVALID_PARAM;

	dot = strchr(full_name, '.');
	if (!dot || dot == full_name || *(dot + 1) == '\0')
		goto out;

	*instance = dot + 1;

	type_name = strndup(full_name, dot - full_name);
	if (!type_name) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	f_type_ret = usbg_lookup_function_type(type_name);

	if (f_type_ret >= 0) {
		*f_type = (usbg_function_type)f_type_ret;
		ret = USBG_SUCCESS;
	} else {
		ret = USBG_ERROR_NOT_SUPPORTED;
	}

out:
	free(type_name);
	return ret;
}

static int usbg_split_config_label_id(const char *full_name,
		char **label)
{
	int ret = USBG_ERROR_INVALID_PARAM;
	char *dot, *endptr, *id_string;

	dot = strrchr(full_name, '.');
	id_string = dot + 1;
	if (!dot || dot == full_name || *id_string == '\0'
			|| isspace(*id_string))
		goto out;

	*label = strndup(full_name, dot - full_name);
	if (!*label)
		goto out;

	errno = 0; /* clear errno */
	ret = strtol(id_string, &endptr, 10);
	if (errno) {
		/* error occurred */
		ret = usbg_translate_error(errno);
		free(*label);
		*label = NULL;
		goto out;
	}

	if (*endptr != '\0' || ret < 0 || ret > 255)
		ret = USBG_ERROR_INVALID_PARAM;

out:
	return ret;
}

static int bindings_select(const struct dirent *dent)
{
	if (dent->d_type == DT_LNK)
		return 1;
	else
		return 0;
}

static inline void usbg_free_binding(usbg_binding *b)
{
	free(b->path);
	free(b->name);
	free(b);
}

static inline void usbg_free_function(usbg_function *f)
{
	free(f->path);
	free(f->name);
	free(f->label);
	free(f);
}

static void usbg_free_config(usbg_config *c)
{
	usbg_binding *b;

	while (!TAILQ_EMPTY(&c->bindings)) {
		b = TAILQ_FIRST(&c->bindings);
		TAILQ_REMOVE(&c->bindings, b, bnode);
		usbg_free_binding(b);
	}
	free(c->path);
	free(c->name);
	free(c->label);
	free(c);
}

static void usbg_free_gadget(usbg_gadget *g)
{
	usbg_config *c;
	usbg_function *f;

	if (g->last_failed_import) {
		config_destroy(g->last_failed_import);
		free(g->last_failed_import);
	}

	while (!TAILQ_EMPTY(&g->configs)) {
		c = TAILQ_FIRST(&g->configs);
		TAILQ_REMOVE(&g->configs, c, cnode);
		usbg_free_config(c);
	}
	while (!TAILQ_EMPTY(&g->functions)) {
		f = TAILQ_FIRST(&g->functions);
		TAILQ_REMOVE(&g->functions, f, fnode);
		usbg_free_function(f);
	}
	free(g->path);
	free(g->name);
	free(g);
}

static void usbg_free_udc(usbg_udc *u)
{
	free(u->name);
	free(u);
}

static void usbg_free_state(usbg_state *s)
{
	usbg_gadget *g;
	usbg_udc *u;

	while (!TAILQ_EMPTY(&s->gadgets)) {
		g = TAILQ_FIRST(&s->gadgets);
		TAILQ_REMOVE(&s->gadgets, g, gnode);
		usbg_free_gadget(g);
	}


	while (!TAILQ_EMPTY(&s->udcs)) {
		u = TAILQ_FIRST(&s->udcs);
		TAILQ_REMOVE(&s->udcs, u, unode);
		usbg_free_udc(u);
	}

	if (s->last_failed_import) {
		config_destroy(s->last_failed_import);
		free(s->last_failed_import);
	}

	free(s->path);
	free(s->configfs_path);
	free(s);
}

static usbg_gadget *usbg_allocate_gadget(const char *path, const char *name,
		usbg_state *parent)
{
	usbg_gadget *g;

	g = malloc(sizeof(*g));
	if (g) {
		TAILQ_INIT(&g->functions);
		TAILQ_INIT(&g->configs);
		g->last_failed_import = NULL;
		g->name = strdup(name);
		g->path = strdup(path);
		g->parent = parent;
		g->udc = NULL;

		if (!(g->name) || !(g->path)) {
			free(g->name);
			free(g->path);
			free(g);
			g = NULL;
		}
	}

	return g;
}

static usbg_config *usbg_allocate_config(const char *path, const char *label,
		int id, usbg_gadget *parent)
{
	usbg_config *c;
	int ret;

	c = malloc(sizeof(*c));
	if (!c)
		goto out;

	TAILQ_INIT(&c->bindings);

	ret = asprintf(&(c->name), "%s.%d", label, id);
	if (ret < 0) {
		free(c);
		c = NULL;
		goto out;
	}

	c->path = strdup(path);
	c->label = strdup(label);
	c->parent = parent;
	c->id = id;

	if (!(c->path) || !(c->label)) {
		free(c->name);
		free(c->path);
		free(c->label);
		free(c);
		c = NULL;
	}

out:
	return c;
}

static usbg_function *usbg_allocate_function(const char *path,
		usbg_function_type type, const char *instance, usbg_gadget *parent)
{
	usbg_function *f;
	const char *type_name;
	int ret;

	f = malloc(sizeof(*f));
	if (!f)
		goto out;

	f->label = NULL;
	type_name = usbg_get_function_type_str(type);
	if (!type_name) {
		free(f);
		f = NULL;
		goto out;
	}

	ret = asprintf(&(f->name), "%s.%s", type_name, instance);
	if (ret < 0) {
		free(f);
		f = NULL;
		goto out;
	}
	f->instance = f->name + strlen(type_name) + 1;
	f->path = strdup(path);
	f->parent = parent;
	f->type = type;
	f->ops = function_types[type];

	if (!(f->path)) {
		free(f->name);
		free(f->path);
		free(f);
		f = NULL;
	}

out:
	return f;
}

static usbg_binding *usbg_allocate_binding(const char *path, const char *name,
		usbg_config *parent)
{
	usbg_binding *b;

	b = malloc(sizeof(*b));
	if (b) {
		b->name = strdup(name);
		b->path = strdup(path);
		b->parent = parent;

		if (!(b->name) || !(b->path)) {
			free(b->name);
			free(b->path);
			free(b);
			b = NULL;
		}
	}

	return b;
}

static usbg_udc *usbg_allocate_udc(usbg_state *parent, const char *name)
{
	usbg_udc *u;

	u = malloc(sizeof(*u));
	if (!u)
		goto out;

	u->gadget = NULL;
	u->parent = parent;
	u->name = strdup(name);
	if (!u->name) {
		free(u);
		u = NULL;
	}

 out:
	return u;
}

char *usbg_ether_ntoa_r(const struct ether_addr *addr, char *buf)
{
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
		addr->ether_addr_octet[0], addr->ether_addr_octet[1],
		addr->ether_addr_octet[2], addr->ether_addr_octet[3],
		addr->ether_addr_octet[4], addr->ether_addr_octet[5]);
	return buf;
}

static int usbg_parse_functions(const char *path, usbg_gadget *g)
{
	usbg_function *f;
	int i, n;
	int ret = USBG_SUCCESS;

	struct dirent **dent;
	char fpath[USBG_MAX_PATH_LENGTH];

	n = snprintf(fpath, sizeof(fpath), "%s/%s/%s", path, g->name,
			FUNCTIONS_DIR);
	if (n >= sizeof(fpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	n = scandir(fpath, &dent, file_select, alphasort);
	if (n < 0) {
		ret = usbg_translate_error(errno);
		goto out;
	}

	for (i = 0; i < n; i++) {
		if (ret == USBG_SUCCESS) {
			const char *instance;
			usbg_function_type type;
			ret = usbg_split_function_instance_type(
					dent[i]->d_name, &type, &instance);
			if (ret == USBG_SUCCESS) {
				f = usbg_allocate_function(fpath, type,
						instance, g);
				if (f)
					TAILQ_INSERT_TAIL(&g->functions, f, fnode);
				else
					ret = USBG_ERROR_NO_MEM;
			}
		}
		free(dent[i]);
	}
	free(dent);

out:
	return ret;
}

static int usbg_parse_config_attrs(const char *path, const char *name,
		usbg_config_attrs *c_attrs)
{
	int buf, ret;

	ret = usbg_read_dec(path, name, "MaxPower", &buf);
	if (ret == USBG_SUCCESS) {
		c_attrs->bMaxPower = (uint8_t)buf;

		ret = usbg_read_hex(path, name, "bmAttributes", &buf);
		if (ret == USBG_SUCCESS)
			c_attrs->bmAttributes = (uint8_t)buf;
	}

	return ret;
}

static int usbg_parse_config_strs(const char *path, const char *name,
		int lang, usbg_config_strs *c_strs)
{
	DIR *dir;
	int ret;
	int nmb;
	char spath[USBG_MAX_PATH_LENGTH];

	nmb = snprintf(spath, sizeof(spath), "%s/%s/%s/0x%x", path, name,
			STRINGS_DIR, lang);
	if (nmb < sizeof(spath)) {
		/* Check if directory exist */
		dir = opendir(spath);
		if (dir) {
			closedir(dir);
			ret = usbg_read_string(spath, "", "configuration",
					c_strs->configuration);
		} else {
			ret = usbg_translate_error(errno);
		}
	} else {
		ret = USBG_ERROR_PATH_TOO_LONG;
	}

	return ret;
}

static int usbg_parse_config_binding(usbg_config *c, char *bpath, int path_size)
{
	int nmb;
	int ret;
	char target[USBG_MAX_PATH_LENGTH];
	char *target_name;
	const char *instance;
	usbg_function_type type;
	usbg_function *f;
	usbg_binding *b;

	nmb = readlink(bpath, target, sizeof(target) - 1 );
	if (nmb < 0) {
		ret = usbg_translate_error(errno);
		goto out;
	}

	/* readlink() don't add this,
	 * so we have to do it manually */
	target[nmb] = '\0';
	/* Target contains a full path
	 * but we need only function dir name */
	target_name = strrchr(target, '/') + 1;

	ret = usbg_split_function_instance_type(target_name, &type, &instance);
	if (ret != USBG_SUCCESS)
		goto out;

	f = usbg_get_function(c->parent, type, instance);
	if (!f) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto out;
	}

	/* We have to cut last part of path */
	bpath[path_size] = '\0';
	/* path_to_config_dir \0 config_name */
	b = usbg_allocate_binding(bpath, bpath + path_size + 1, c);
	if (b) {
		b->target = f;
		TAILQ_INSERT_TAIL(&c->bindings, b, bnode);
	} else {
		ret = USBG_ERROR_NO_MEM;
	}

out:
	return ret;
}

static int usbg_parse_config_bindings(usbg_config *c)
{
	int i, n, nmb;
	int ret = USBG_SUCCESS;
	struct dirent **dent;
	char bpath[USBG_MAX_PATH_LENGTH];
	int end;

	end = snprintf(bpath, sizeof(bpath), "%s/%s", c->path, c->name);
	if (end >= sizeof(bpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	n = scandir(bpath, &dent, bindings_select, alphasort);
	if (n < 0) {
		ret = usbg_translate_error(errno);
		goto out;
	}

	for (i = 0; i < n; i++) {
		if (ret == USBG_SUCCESS) {
			nmb = snprintf(&(bpath[end]), sizeof(bpath) - end,
					"/%s", dent[i]->d_name);

			ret = nmb < sizeof(bpath) - end ?
					usbg_parse_config_binding(c, bpath, end)
					: USBG_ERROR_PATH_TOO_LONG;
		} /* ret == USBG_SUCCESS */
		free(dent[i]);
	}
	free(dent);

out:
	return ret;
}

static int usbg_parse_config(const char *path, const char *name,
		usbg_gadget *g)
{
	int ret;
	char *label = NULL;
	usbg_config *c;

	ret = usbg_split_config_label_id(name, &label);
	if (ret <= 0)
		goto out;

	c = usbg_allocate_config(path, label, ret, g);
	if (!c) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	ret = usbg_parse_config_bindings(c);
	if (ret == USBG_SUCCESS)
		TAILQ_INSERT_TAIL(&g->configs, c, cnode);
	else
		usbg_free_config(c);

out:
	free(label);
	return ret;
}

static int usbg_parse_configs(const char *path, usbg_gadget *g)
{
	int i, n;
	int ret = USBG_SUCCESS;
	struct dirent **dent;
	char cpath[USBG_MAX_PATH_LENGTH];

	n = snprintf(cpath, sizeof(cpath), "%s/%s/%s", path, g->name,
			CONFIGS_DIR);
	if (n >= sizeof(cpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	n = scandir(cpath, &dent, file_select, alphasort);
	if (n < 0) {
		ret = usbg_translate_error(errno);
		goto out;
	}

	for (i = 0; i < n; i++) {
		ret = ret == USBG_SUCCESS ?
				usbg_parse_config(cpath, dent[i]->d_name, g)
				: ret;
		free(dent[i]);
	}
	free(dent);

out:
	return ret;
}

static int usbg_parse_gadget_attrs(const char *path, const char *name,
		usbg_gadget_attrs *g_attrs)
{
	int buf, ret;

	/* Actual attributes */

	ret = usbg_read_hex(path, name, "bcdUSB", &buf);
	if (ret == USBG_SUCCESS)
		g_attrs->bcdUSB = (uint16_t) buf;
	else
		goto out;

	ret = usbg_read_hex(path, name, "bDeviceClass", &buf);
	if (ret == USBG_SUCCESS)
		g_attrs->bDeviceClass = (uint8_t)buf;
	else
		goto out;

	ret = usbg_read_hex(path, name, "bDeviceSubClass", &buf);
	if (ret == USBG_SUCCESS)
		g_attrs->bDeviceSubClass = (uint8_t)buf;
	else
		goto out;

	ret = usbg_read_hex(path, name, "bDeviceProtocol", &buf);
	if (ret == USBG_SUCCESS)
		g_attrs->bDeviceProtocol = (uint8_t) buf;
	else
		goto out;

	ret = usbg_read_hex(path, name, "bMaxPacketSize0", &buf);
	if (ret == USBG_SUCCESS)
		g_attrs->bMaxPacketSize0 = (uint8_t) buf;
	else
		goto out;

	ret = usbg_read_hex(path, name, "idVendor", &buf);
	if (ret == USBG_SUCCESS)
		g_attrs->idVendor = (uint16_t) buf;
	else
		goto out;

	ret = usbg_read_hex(path, name, "idProduct", &buf);
	if (ret == USBG_SUCCESS)
		g_attrs->idProduct = (uint16_t) buf;
	else
		goto out;

	ret = usbg_read_hex(path, name, "bcdDevice", &buf);
	if (ret == USBG_SUCCESS)
		g_attrs->bcdDevice = (uint16_t) buf;
	else
		goto out;

out:
	return ret;
}

static int usbg_parse_gadget_strs(const char *path, const char *name, int lang,
		usbg_gadget_strs *g_strs)
{
	int ret;
	int nmb;
	DIR *dir;
	char spath[USBG_MAX_PATH_LENGTH];

	nmb = snprintf(spath, sizeof(spath), "%s/%s/%s/0x%x", path, name,
			STRINGS_DIR, lang);
	if (nmb >= sizeof(spath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	/* Check if directory exist */
	dir = opendir(spath);
	if (dir) {
		closedir(dir);
		ret = usbg_read_string(spath, "", "serialnumber", g_strs->str_ser);
		if (ret != USBG_SUCCESS)
			goto out;

		ret = usbg_read_string(spath, "", "manufacturer", g_strs->str_mnf);
		if (ret != USBG_SUCCESS)
			goto out;

		ret = usbg_read_string(spath, "", "product", g_strs->str_prd);
		if (ret != USBG_SUCCESS)
			goto out;
	} else {
		ret = usbg_translate_error(errno);
	}

out:
	return ret;
}

static inline int usbg_parse_gadget(usbg_gadget *g)
{
	int ret;
	char buf[USBG_MAX_STR_LENGTH];

	/* UDC bound to, if any */
	ret = usbg_read_string(g->path, g->name, "UDC", buf);
	if (ret != USBG_SUCCESS)
		goto out;

	g->udc = usbg_get_udc(g->parent, buf);
	if (g->udc)
		g->udc->gadget = g;

	ret = usbg_parse_functions(g->path, g);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_parse_configs(g->path, g);
out:
	return ret;
}

static int usbg_parse_gadgets(const char *path, usbg_state *s)
{
	usbg_gadget *g;
	int i, n;
	int ret = USBG_SUCCESS;
	struct dirent **dent;

	n = scandir(path, &dent, file_select, alphasort);
	if (n >= 0) {
		for (i = 0; i < n; i++) {
			/* Check if earlier gadgets
			 * has been created correctly */
			if (ret == USBG_SUCCESS) {
				/* Create new gadget and insert it into list */
				g = usbg_allocate_gadget(path, dent[i]->d_name, s);
				if (g) {
					ret = usbg_parse_gadget(g);
					if (ret == USBG_SUCCESS)
						TAILQ_INSERT_TAIL(&s->gadgets, g, gnode);
					else
						usbg_free_gadget(g);
				} else {
					ret = USBG_ERROR_NO_MEM;
				}
			}
			free(dent[i]);
		}
		free(dent);
	} else {
		ret = usbg_translate_error(errno);
	}

	return ret;
}

static int usbg_parse_udcs(usbg_state *s)
{
	usbg_udc *u;
	int n, i;
	int ret = USBG_SUCCESS;
	struct dirent **dent;

	n = scandir("/sys/class/udc", &dent, file_select, alphasort);
	if (n < 0) {
		ret = usbg_translate_error(errno);
		goto out;
	}

	for (i = 0; i < n; ++i) {
		if (ret == USBG_SUCCESS) {
			u = usbg_allocate_udc(s, dent[i]->d_name);
			if (u)
				TAILQ_INSERT_TAIL(&s->udcs, u, unode);
			else
				ret = USBG_ERROR_NO_MEM;
		}

		free(dent[i]);
	}
	free(dent);

out:
	return ret;
}

static usbg_state *usbg_allocate_state(const char *configfs_path, char *path)
{
	usbg_state *s;

	s = malloc(sizeof(*s));
	if (!s)
		goto err;

	s->configfs_path = strdup(configfs_path);
	if (!s->configfs_path)
		goto cpath_failed;

	/* State takes the ownership of path and should free it */
	s->path = path;
	s->last_failed_import = NULL;
	TAILQ_INIT(&s->gadgets);
	TAILQ_INIT(&s->udcs);

	return s;

cpath_failed:
	free(s);
err:
	return NULL;
}

static int usbg_parse_state(usbg_state *s)
{
	int ret = USBG_SUCCESS;

	/*
	 * USBG_ERROR_NOT_FOUND is returned if we are running on machine where
	 * there is no udc support in kernel (no /sys/class/udc dir).
	 * This check allows to run library on such machine or if we don't
	 * have rights to read this directory.
	 * User will be able to finish init function and manage gadgets but
	 * wont be able to bind it as there is no UDC.
	 */
	ret = usbg_parse_udcs(s);
	if (ret != USBG_SUCCESS && ret != USBG_ERROR_NOT_FOUND &&
		ret != USBG_ERROR_NO_ACCESS) {
		ERROR("Unable to parse udcs");
		goto out;
	}

	ret = usbg_parse_gadgets(s->path, s);
	if (ret != USBG_SUCCESS)
		ERROR("unable to parse %s\n", s->path);

out:
	return ret;
}

/*
 * User API
 */

int usbg_init(const char *configfs_path, usbg_state **state)
{
	int ret = USBG_SUCCESS;
	DIR *dir;
	char *path;
	usbg_state *s;

	ret = asprintf(&path, "%s/" GADGETS_DIR, configfs_path);
	if (ret < 0)
		return USBG_ERROR_NO_MEM;
	else
		ret = USBG_SUCCESS;

	/* Check if directory exist */
	dir = opendir(path);
	if (!dir) {
		ERRORNO("couldn't init gadget state\n");
		ret = usbg_translate_error(errno);
		goto err;
	}

	closedir(dir);
	s = usbg_allocate_state(configfs_path, path);
	if (!s) {
		ret = USBG_ERROR_NO_MEM;
		goto err;
	}

	ret = usbg_parse_state(s);
	if (ret != USBG_SUCCESS) {
		ERROR("couldn't init gadget state\n");
		usbg_free_state(s);
		goto out;
	}

	*state = s;

	return ret;

err:
	free(path);
out:
	return ret;
}

void usbg_cleanup(usbg_state *s)
{
	usbg_free_state(s);
}

const char *usbg_get_configfs_path(usbg_state *s)
{
	return s ? s->configfs_path : NULL;
}

size_t usbg_get_configfs_path_len(usbg_state *s)
{
	return s ? strlen(s->configfs_path) : USBG_ERROR_INVALID_PARAM;
}

int usbg_cpy_configfs_path(usbg_state *s, char *buf, size_t len)
{
	if (!s || !buf || len == 0)
		return USBG_ERROR_INVALID_PARAM;

	buf[--len] = '\0';
	strncpy(buf, s->configfs_path, len);

	return USBG_SUCCESS;
}

usbg_gadget *usbg_get_gadget(usbg_state *s, const char *name)
{
	usbg_gadget *g;

	TAILQ_FOREACH(g, &s->gadgets, gnode)
		if (!strcmp(g->name, name))
			return g;

	return NULL;
}

usbg_function *usbg_get_function(usbg_gadget *g,
		usbg_function_type type, const char *instance)
{
	usbg_function *f = NULL;

	TAILQ_FOREACH(f, &g->functions, fnode)
		if (f->type == type && (!strcmp(f->instance, instance)))
			break;

	return f;
}

usbg_config *usbg_get_config(usbg_gadget *g, int id, const char *label)
{
	usbg_config *c = NULL;

	TAILQ_FOREACH(c, &g->configs, cnode)
		if (c->id == id && (!label || !strcmp(c->label, label)))
			break;

	return c;
}

usbg_udc *usbg_get_udc(usbg_state *s, const char *name)
{
	usbg_udc *u;

	TAILQ_FOREACH(u, &s->udcs, unode)
		if (!strcmp(u->name, name))
			return u;

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

int usbg_rm_binding(usbg_binding *b)
{
	int ret = USBG_SUCCESS;
	usbg_config *c;

	if (!b)
		return USBG_ERROR_INVALID_PARAM;

	c = b->parent;

	ret = ubsg_rm_file(b->path, b->name);
	if (ret == USBG_SUCCESS) {
		TAILQ_REMOVE(&(c->bindings), b, bnode);
		usbg_free_binding(b);
	}

	return ret;
}

int usbg_rm_config(usbg_config *c, int opts)
{
	int ret = USBG_ERROR_INVALID_PARAM;
	usbg_gadget *g;

	if (!c)
		return ret;

	g = c->parent;

	if (opts & USBG_RM_RECURSE) {
		/* Recursive flag was given
		 * so remove all bindings and strings */
		char spath[USBG_MAX_PATH_LENGTH];
		int nmb;
		usbg_binding *b;

		while (!TAILQ_EMPTY(&c->bindings)) {
			b = TAILQ_FIRST(&c->bindings);
			ret = usbg_rm_binding(b);
			if (ret != USBG_SUCCESS)
				goto out;
		}

		nmb = snprintf(spath, sizeof(spath), "%s/%s/%s", c->path,
				c->name, STRINGS_DIR);
		if (nmb >= sizeof(spath)) {
			ret = USBG_ERROR_PATH_TOO_LONG;
			goto out;
		}

		ret = usbg_rm_all_dirs(spath);
		if (ret != USBG_SUCCESS)
			goto out;
	}

	ret = usbg_rm_dir(c->path, c->name);
	if (ret == USBG_SUCCESS) {
		TAILQ_REMOVE(&(g->configs), c, cnode);
		usbg_free_config(c);
	}

out:
	return ret;
}

int usbg_rm_function(usbg_function *f, int opts)
{
	int ret = USBG_ERROR_INVALID_PARAM;
	usbg_gadget *g;

	if (!f)
		return ret;

	g = f->parent;

	if (opts & USBG_RM_RECURSE) {
		/* Recursive flag was given
		 * so remove all bindings to this function */
		usbg_config *c;
		usbg_binding *b;

		TAILQ_FOREACH(c, &g->configs, cnode) {
			b = TAILQ_FIRST(&c->bindings);
			while (b != NULL) {
				if (b->target == f) {
					usbg_binding *b_next = TAILQ_NEXT(b, bnode);
					ret = usbg_rm_binding(b);
					if (ret != USBG_SUCCESS)
						return ret;

					b = b_next;
				} else {
					b = TAILQ_NEXT(b, bnode);
				}
			} /* while */
		} /* TAILQ_FOREACH */
	}

	if (f->ops->remove) {
		ret = f->ops->remove(f, opts);
		if (ret != USBG_SUCCESS)
			goto out;
	}

	ret = usbg_rm_dir(f->path, f->name);
	if (ret == USBG_SUCCESS) {
		TAILQ_REMOVE(&(g->functions), f, fnode);
		usbg_free_function(f);
	}

out:
	return ret;
}

int usbg_rm_gadget(usbg_gadget *g, int opts)
{
	int ret = USBG_ERROR_INVALID_PARAM;
	usbg_state *s;
	if (!g)
		goto out;

	s = g->parent;

	if (opts & USBG_RM_RECURSE) {
		/* Recursive flag was given
		 * so remove all configs and functions
		 * using recursive flags */
		usbg_config *c;
		usbg_function *f;
		int nmb;
		char spath[USBG_MAX_PATH_LENGTH];

		while (!TAILQ_EMPTY(&g->configs)) {
			c = TAILQ_FIRST(&g->configs);
			ret = usbg_rm_config(c, opts);
			if (ret != USBG_SUCCESS)
				goto out;
		}

		while (!TAILQ_EMPTY(&g->functions)) {
			f = TAILQ_FIRST(&g->functions);
			ret = usbg_rm_function(f, opts);
			if (ret != USBG_SUCCESS)
				goto out;
		}

		nmb = snprintf(spath, sizeof(spath), "%s/%s/%s", g->path,
				g->name, STRINGS_DIR);
		if (nmb >= sizeof(spath)) {
			ret = USBG_ERROR_PATH_TOO_LONG;
			goto out;
		}

		ret = usbg_rm_all_dirs(spath);
		if (ret != USBG_SUCCESS)
			goto out;
	}

	ret = usbg_rm_dir(g->path, g->name);
	if (ret == USBG_SUCCESS) {
		TAILQ_REMOVE(&(s->gadgets), g, gnode);
		usbg_free_gadget(g);
	}

out:
	return ret;
}

int usbg_rm_config_strs(usbg_config *c, int lang)
{
	int ret = USBG_SUCCESS;
	int nmb;
	char path[USBG_MAX_PATH_LENGTH];

	if (!c)
		return USBG_ERROR_INVALID_PARAM;

	nmb = snprintf(path, sizeof(path), "%s/%s/%s/0x%x", c->path, c->name,
			STRINGS_DIR, lang);
	if (nmb < sizeof(path))
		ret = usbg_rm_dir(path, "");
	else
		ret = USBG_ERROR_PATH_TOO_LONG;

	return ret;
}

int usbg_rm_gadget_strs(usbg_gadget *g, int lang)
{
	int ret = USBG_SUCCESS;
	int nmb;
	char path[USBG_MAX_PATH_LENGTH];

	if (!g)
		return USBG_ERROR_INVALID_PARAM;

	nmb = snprintf(path, sizeof(path), "%s/%s/%s/0x%x", g->path, g->name,
			STRINGS_DIR, lang);
	if (nmb < sizeof(path))
		ret = usbg_rm_dir(path, "");
	else
		ret = USBG_ERROR_PATH_TOO_LONG;

	return ret;
}


static int usbg_create_empty_gadget(usbg_state *s, const char *name,
				    usbg_gadget **g)
{
	char gpath[USBG_MAX_PATH_LENGTH];
	char buf[USBG_MAX_STR_LENGTH];
	int nmb;
	usbg_gadget *gad;
	int ret = USBG_SUCCESS;

	nmb = snprintf(gpath, sizeof(gpath), "%s/%s", s->path, name);
	if (nmb >= sizeof(gpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	*g = usbg_allocate_gadget(s->path, name, s);
	if (!*g) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	gad = *g; /* alias only */

	ret = mkdir(gpath, S_IRWXU|S_IRWXG|S_IRWXO);
	if (ret == 0) {
		/* Should be empty but read the default */
		ret = usbg_read_string(gad->path, gad->name,
				       "UDC", buf);
		if (ret != USBG_SUCCESS) {
			rmdir(gpath);
		} else {
			gad->udc = usbg_get_udc(s, buf);
			if (gad->udc)
				gad->udc->gadget = gad;
		}
	} else {
		ret = usbg_translate_error(errno);
	}

	if (ret != USBG_SUCCESS) {
		usbg_free_gadget(*g);
		*g = NULL;
	}

out:
	return ret;
}

int usbg_create_gadget_vid_pid(usbg_state *s, const char *name,
		uint16_t idVendor, uint16_t idProduct, usbg_gadget **g)
{
	int ret;
	usbg_gadget *gad;

	if (!s || !g)
		return USBG_ERROR_INVALID_PARAM;

	gad = usbg_get_gadget(s, name);
	if (gad) {
		ERROR("duplicate gadget name\n");
		return USBG_ERROR_EXIST;
	}

	ret = usbg_create_empty_gadget(s, name, g);
	gad = *g;

	/* Check if gadget creation was successful and set attributes */
	if (ret == USBG_SUCCESS) {
		ret = usbg_write_hex16(s->path, name, "idVendor", idVendor);
		if (ret == USBG_SUCCESS) {
			ret = usbg_write_hex16(s->path, name, "idProduct", idProduct);
			if (ret == USBG_SUCCESS)
				INSERT_TAILQ_STRING_ORDER(&s->gadgets, ghead, name,
						gad, gnode);
			else
				usbg_free_gadget(gad);
		}
	}

	return ret;
}

int usbg_create_gadget(usbg_state *s, const char *name,
		       const usbg_gadget_attrs *g_attrs, const usbg_gadget_strs *g_strs,
		       usbg_gadget **g)
{
	usbg_gadget *gad;
	int ret;

	if (!s || !g)
			return USBG_ERROR_INVALID_PARAM;

	gad = usbg_get_gadget(s, name);
	if (gad) {
		ERROR("duplicate gadget name\n");
		return USBG_ERROR_EXIST;
	}

	ret = usbg_create_empty_gadget(s, name, g);
	gad = *g;

	/* Check if gadget creation was successful and set attrs and strings */
	if (ret == USBG_SUCCESS) {
		if (g_attrs)
			ret = usbg_set_gadget_attrs(gad, g_attrs);

		if (g_strs)
			ret = usbg_set_gadget_strs(gad, LANG_US_ENG, g_strs);

		if (ret == USBG_SUCCESS)
			INSERT_TAILQ_STRING_ORDER(&s->gadgets, ghead, name,
				gad, gnode);
		else
			usbg_free_gadget(gad);
	}
	return ret;
}

int usbg_get_gadget_attrs(usbg_gadget *g, usbg_gadget_attrs *g_attrs)
{
	return g && g_attrs ? usbg_parse_gadget_attrs(g->path, g->name, g_attrs)
			: USBG_ERROR_INVALID_PARAM;
}

const char *usbg_get_gadget_name(usbg_gadget *g)
{
	return g ? g->name : NULL;
}

size_t usbg_get_gadget_name_len(usbg_gadget *g)
{
	return g ? strlen(g->name) : USBG_ERROR_INVALID_PARAM;
}

int usbg_cpy_gadget_name(usbg_gadget *g, char *buf, size_t len)
{
	if (!g || !buf || len == 0)
		return USBG_ERROR_INVALID_PARAM;

	buf[--len] = '\0';
	strncpy(buf, g->name, len);

	return USBG_SUCCESS;
}

const char *usbg_get_udc_name(usbg_udc *u)
{
	return u ? u->name : NULL;
}

size_t usbg_get_udc_name_len(usbg_udc *u)
{
	return u ? strlen(u->name) : USBG_ERROR_INVALID_PARAM;
}

int usbg_cpy_udc_name(usbg_udc *u, char *buf, size_t len)
{
	if (!u || !buf || len == 0)
		return USBG_ERROR_INVALID_PARAM;

	buf[--len] = '\0';
	strncpy(buf, u->name, len);

	return USBG_SUCCESS;
}

int usbg_set_gadget_attr(usbg_gadget *g, usbg_gadget_attr attr, int val)
{
	const char *attr_name;
	int ret = USBG_ERROR_INVALID_PARAM;

	if (!g)
		goto out;

	attr_name = usbg_get_gadget_attr_str(attr);
	if (!attr_name)
		goto out;

	ret = usbg_write_hex(g->path, g->name, attr_name, val);

out:
	return ret;
}

int usbg_get_gadget_attr(usbg_gadget *g, usbg_gadget_attr attr)
{
	const char *attr_name;
	int ret = USBG_ERROR_INVALID_PARAM;

	if (!g)
		goto out;

	attr_name = usbg_get_gadget_attr_str(attr);
	if (!attr_name)
		goto out;

	usbg_read_hex(g->path, g->name, attr_name, &ret);

out:
	return ret;
}

usbg_udc *usbg_get_gadget_udc(usbg_gadget *g)
{
	usbg_udc *u = NULL;

	if (!g)
		goto out;
	/*
	 * if gadget was enabled we have to check if kernel
	 * didn't modify the UDC file due to some errors.
	 * For example some FFS daemon could just get
	 * a segmentation fault or sth
	 */
	if (g->udc) {
		char buf[USBG_MAX_STR_LENGTH];
		int ret;

		ret = usbg_read_string(g->path, g->name, "UDC", buf);
		if (ret != USBG_SUCCESS)
			goto out;

		if (!strcmp(g->udc->name, buf)) {
			/* Gadget is still assigned to this UDC */
			u = g->udc;
		} else {
			/* Kernel decided to detach this gadget */
			g->udc->gadget = NULL;
			g->udc = NULL;
		}
	}

out:
	return u;
}

usbg_gadget *usbg_get_udc_gadget(usbg_udc *u)
{
	usbg_gadget *g = NULL;

	if (!u)
		goto out;
	/*
	 * if gadget was enabled on this UDC we have to check if kernel
	 * didn't modify this due to some errors.
	 * For example some FFS daemon could just get a segmentation fault
	 * what causes detach of gadget
	 */
	if (u->gadget) {
		usbg_udc *u_checked;

		u_checked = usbg_get_gadget_udc(u->gadget);
		if (u_checked) {
			g = u->gadget;
		} else {
			u->gadget->udc = NULL;
			u->gadget = NULL;
		}
	}

out:
	return g;
}

int usbg_set_gadget_attrs(usbg_gadget *g, const usbg_gadget_attrs *g_attrs)
{
	int ret;
	if (!g || !g_attrs)
		return USBG_ERROR_INVALID_PARAM;

	ret = usbg_write_hex16(g->path, g->name, "bcdUSB", g_attrs->bcdUSB);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_hex8(g->path, g->name, "bDeviceClass",
		g_attrs->bDeviceClass);
	if (ret != USBG_SUCCESS)
			goto out;

	ret = usbg_write_hex8(g->path, g->name, "bDeviceSubClass",
		g_attrs->bDeviceSubClass);
	if (ret != USBG_SUCCESS)
			goto out;

	ret = usbg_write_hex8(g->path, g->name, "bDeviceProtocol",
		g_attrs->bDeviceProtocol);
	if (ret != USBG_SUCCESS)
			goto out;

	ret = usbg_write_hex8(g->path, g->name, "bMaxPacketSize0",
		g_attrs->bMaxPacketSize0);
	if (ret != USBG_SUCCESS)
			goto out;

	ret = usbg_write_hex16(g->path, g->name, "idVendor",
		g_attrs->idVendor);
	if (ret != USBG_SUCCESS)
			goto out;

	ret = usbg_write_hex16(g->path, g->name, "idProduct",
		 g_attrs->idProduct);
	if (ret != USBG_SUCCESS)
			goto out;

	ret = usbg_write_hex16(g->path, g->name, "bcdDevice",
		g_attrs->bcdDevice);

out:
	return ret;
}

int usbg_set_gadget_vendor_id(usbg_gadget *g, uint16_t idVendor)
{
	return g ? usbg_write_hex16(g->path, g->name, "idVendor", idVendor)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_gadget_product_id(usbg_gadget *g, uint16_t idProduct)
{
	return g ? usbg_write_hex16(g->path, g->name, "idProduct", idProduct)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_gadget_device_class(usbg_gadget *g, uint8_t bDeviceClass)
{
	return g ? usbg_write_hex8(g->path, g->name, "bDeviceClass", bDeviceClass)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_gadget_device_protocol(usbg_gadget *g, uint8_t bDeviceProtocol)
{
	return g ? usbg_write_hex8(g->path, g->name, "bDeviceProtocol", bDeviceProtocol)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_gadget_device_subclass(usbg_gadget *g, uint8_t bDeviceSubClass)
{
	return g ? usbg_write_hex8(g->path, g->name, "bDeviceSubClass", bDeviceSubClass)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_gadget_device_max_packet(usbg_gadget *g, uint8_t bMaxPacketSize0)
{
	return g ? usbg_write_hex8(g->path, g->name, "bMaxPacketSize0", bMaxPacketSize0)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_gadget_device_bcd_device(usbg_gadget *g, uint16_t bcdDevice)
{
	return g ? usbg_write_hex16(g->path, g->name, "bcdDevice", bcdDevice)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_gadget_device_bcd_usb(usbg_gadget *g, uint16_t bcdUSB)
{
	return g ? usbg_write_hex16(g->path, g->name, "bcdUSB", bcdUSB)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_get_gadget_strs(usbg_gadget *g, int lang,
		usbg_gadget_strs *g_strs)
{
	return g && g_strs ? usbg_parse_gadget_strs(g->path, g->name, lang,
			g_strs)	: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_gadget_str(usbg_gadget *g, usbg_gadget_str str, int lang,
		const char *val)
{
	const char *str_name;
	int ret = USBG_ERROR_INVALID_PARAM;
	char path[USBG_MAX_PATH_LENGTH];
	int nmb;

	if (!g)
		goto out;

	str_name = usbg_get_gadget_str_name(str);
	if (!str_name)
		goto out;

	nmb = snprintf(path, sizeof(path), "%s/%s/%s/0x%x", g->path, g->name,
			STRINGS_DIR, lang);
	if (nmb >= sizeof(path)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = usbg_check_dir(path);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_string(path, "", str_name, val);

out:
	return ret;
}

int usbg_set_gadget_strs(usbg_gadget *g, int lang,
		const usbg_gadget_strs *g_strs)
{
	char path[USBG_MAX_PATH_LENGTH];
	int nmb;
	int ret = USBG_ERROR_INVALID_PARAM;

	if (!g || !g_strs)
		goto out;

	nmb = snprintf(path, sizeof(path), "%s/%s/%s/0x%x", g->path, g->name,
			STRINGS_DIR, lang);
	if (nmb >= sizeof(path)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = usbg_check_dir(path);
	if (ret == USBG_SUCCESS) {
		ret = usbg_write_string(path, "", "serialnumber", g_strs->str_ser);
		if (ret != USBG_SUCCESS)
			goto out;

		ret = usbg_write_string(path, "", "manufacturer", g_strs->str_mnf);
		if (ret != USBG_SUCCESS)
			goto out;

		ret = usbg_write_string(path, "", "product", g_strs->str_prd);
	}

out:
	return ret;
}

int usbg_set_gadget_serial_number(usbg_gadget *g, int lang, const char *serno)
{
	int ret = USBG_ERROR_INVALID_PARAM;

	if (g && serno) {
		char path[USBG_MAX_PATH_LENGTH];
		int nmb;
		nmb = snprintf(path, sizeof(path), "%s/%s/%s/0x%x", g->path,
				g->name, STRINGS_DIR, lang);
		if (nmb < sizeof(path)) {
			ret = usbg_check_dir(path);
			if (ret == USBG_SUCCESS)
				ret = usbg_write_string(path, "", "serialnumber", serno);
		} else {
			ret = USBG_ERROR_PATH_TOO_LONG;
		}
	}

	return ret;
}

int usbg_set_gadget_manufacturer(usbg_gadget *g, int lang, const char *mnf)
{
	int ret = USBG_ERROR_INVALID_PARAM;

	if (g && mnf) {
		char path[USBG_MAX_PATH_LENGTH];
		int nmb;
		nmb = snprintf(path, sizeof(path), "%s/%s/%s/0x%x", g->path,
				g->name, STRINGS_DIR, lang);
		if (nmb < sizeof(path)) {
			ret = usbg_check_dir(path);
			if (ret == USBG_SUCCESS)
				ret = usbg_write_string(path, "", "manufacturer", mnf);
		} else {
			ret = USBG_ERROR_PATH_TOO_LONG;
		}
	}

	return ret;
}

int usbg_set_gadget_product(usbg_gadget *g, int lang, const char *prd)
{
	int ret = USBG_ERROR_INVALID_PARAM;

	if (g && prd) {
		char path[USBG_MAX_PATH_LENGTH];
		int nmb;
		nmb = snprintf(path, sizeof(path), "%s/%s/%s/0x%x", g->path,
				g->name, STRINGS_DIR, lang);
		if (nmb < sizeof(path)) {
			ret = usbg_check_dir(path);
			if (ret == USBG_SUCCESS)
				ret = usbg_write_string(path, "", "product", prd);
		} else {
			ret = USBG_ERROR_PATH_TOO_LONG;
		}
	}

	return ret;
}

int usbg_create_function(usbg_gadget *g, usbg_function_type type,
			 const char *instance, const usbg_function_attrs *f_attrs,
			 usbg_function **f)
{
	char fpath[USBG_MAX_PATH_LENGTH];
	usbg_function *func;
	int ret = USBG_ERROR_INVALID_PARAM;
	int n, free_space;

	if (!g || !f)
		return ret;

	if (!instance) {
		/* If someone creates ffs function and doesn't pass instance name
		   this means that device name from attrs should be used */
		if (type == F_FFS && f_attrs && f_attrs->attrs.ffs.dev_name) {
			instance = f_attrs->attrs.ffs.dev_name;
			f_attrs = NULL;
		} else {
			return ret;
		}
	}

	func = usbg_get_function(g, type, instance);
	if (func) {
		ERROR("duplicate function name\n");
		ret = USBG_ERROR_EXIST;
		goto out;
	}

	n = snprintf(fpath, sizeof(fpath), "%s/%s/%s", g->path, g->name,
			FUNCTIONS_DIR);
	if (n >= sizeof(fpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	*f = usbg_allocate_function(fpath, type, instance, g);
	func = *f;
	if (!func) {
		ERROR("allocating function\n");
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	free_space = sizeof(fpath) - n;
	n = snprintf(&(fpath[n]), free_space, "/%s", func->name);
	if (n >= free_space) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto free_func;
	}

	ret = mkdir(fpath, S_IRWXU | S_IRWXG | S_IRWXO);
	if (ret) {
		ret = usbg_translate_error(errno);
		goto free_func;
	}

	if (f_attrs) {
		ret = usbg_set_function_attrs(func, f_attrs);
		if (ret != USBG_SUCCESS)
			goto remove_dir;
	}

	INSERT_TAILQ_STRING_ORDER(&g->functions, fhead, name, func, fnode);

	return USBG_SUCCESS;

remove_dir:
	usbg_rm_dir(fpath, "");
free_func:
	usbg_free_function(func);
out:
	return ret;
}

int usbg_create_config(usbg_gadget *g, int id, const char *label,
		const usbg_config_attrs *c_attrs, const usbg_config_strs *c_strs,
		usbg_config **c)
{
	char cpath[USBG_MAX_PATH_LENGTH];
	usbg_config *conf = NULL;
	int ret = USBG_ERROR_INVALID_PARAM;
	int n, free_space;

	if (!g || !c || id <= 0 || id > 255)
		goto out;

	if (!label)
		label = DEFAULT_CONFIG_LABEL;

	conf = usbg_get_config(g, id, NULL);
	if (conf) {
		ERROR("duplicate configuration id\n");
		ret = USBG_ERROR_EXIST;
		goto out;
	}

	n = snprintf(cpath, sizeof(cpath), "%s/%s/%s", g->path, g->name,
			CONFIGS_DIR);
	if (n >= sizeof(cpath)) {
		ret =  USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	*c = usbg_allocate_config(cpath, label, id, g);
	conf = *c;
	if (!conf) {
		ERROR("allocating configuration\n");
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	free_space = sizeof(cpath) - n;
	/* Append string at the end of previous one */
	n = snprintf(&(cpath[n]), free_space, "/%s", (*c)->name);
	if (n >= free_space) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		usbg_free_config(conf);
		goto out;
	}

	ret = mkdir(cpath, S_IRWXU | S_IRWXG | S_IRWXO);
	if (!ret) {
		ret = USBG_SUCCESS;
		if (c_attrs)
			ret = usbg_set_config_attrs(conf, c_attrs);

		if (ret == USBG_SUCCESS && c_strs)
			ret = usbg_set_config_string(conf, LANG_US_ENG,
					c_strs->configuration);
	} else {
		ret = usbg_translate_error(errno);
	}

	if (ret == USBG_SUCCESS)
		INSERT_TAILQ_STRING_ORDER(&g->configs, chead, name,
				conf, cnode);
	else
		usbg_free_config(conf);

out:
	return ret;
}

const char *usbg_get_config_label(usbg_config *c)
{
	return c ? c->label : NULL;
}

size_t usbg_get_config_label_len(usbg_config *c)
{
	return c ? strlen(c->label) : USBG_ERROR_INVALID_PARAM;
}

int usbg_cpy_config_label(usbg_config *c, char *buf, size_t len)
{
	if (!c || !buf || len == 0)
		return USBG_ERROR_INVALID_PARAM;

	buf[--len] = '\0';
	strncpy(buf, c->label, len);

	return USBG_SUCCESS;
}

int usbg_get_config_id(usbg_config *c)
{
	return c ? c->id : USBG_ERROR_INVALID_PARAM;
}

const char *usbg_get_function_instance(usbg_function *f)
{
	return f ? f->instance : NULL;
}

size_t usbg_get_function_instance_len(usbg_function *f)
{
	return f ? strlen(f->instance) : USBG_ERROR_INVALID_PARAM;
}

int usbg_cpy_function_instance(usbg_function *f, char *buf, size_t len)
{
	if (!f || !buf || len == 0)
		return USBG_ERROR_INVALID_PARAM;

	buf[--len] = '\0';
	strncpy(buf, f->instance, len);

	return USBG_SUCCESS;
}

int usbg_set_config_attrs(usbg_config *c, const usbg_config_attrs *c_attrs)
{
	int ret = USBG_ERROR_INVALID_PARAM;

	if (c && c_attrs) {
		ret = usbg_write_dec(c->path, c->name, "MaxPower", c_attrs->bMaxPower);
		if (ret == USBG_SUCCESS)
			ret = usbg_write_hex8(c->path, c->name, "bmAttributes",
					c_attrs->bmAttributes);
	}

	return ret;
}

int usbg_get_config_attrs(usbg_config *c,
		usbg_config_attrs *c_attrs)
{
	return c && c_attrs ? usbg_parse_config_attrs(c->path, c->name, c_attrs)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_config_max_power(usbg_config *c, int bMaxPower)
{
	return c ? usbg_write_dec(c->path, c->name, "MaxPower", bMaxPower)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_config_bm_attrs(usbg_config *c, int bmAttributes)
{
	return c ? usbg_write_hex8(c->path, c->name, "bmAttributes", bmAttributes)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_get_config_strs(usbg_config *c, int lang, usbg_config_strs *c_strs)
{
	return c && c_strs ? usbg_parse_config_strs(c->path, c->name, lang, c_strs)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_set_config_strs(usbg_config *c, int lang,
		const usbg_config_strs *c_strs)
{
	return usbg_set_config_string(c, lang, c_strs->configuration);
}

int usbg_set_config_string(usbg_config *c, int lang, const char *str)
{
	int ret = USBG_ERROR_INVALID_PARAM;

	if (c && str) {
		char path[USBG_MAX_PATH_LENGTH];
		int nmb;
		nmb = snprintf(path, sizeof(path), "%s/%s/%s/0x%x", c->path,
				c->name, STRINGS_DIR, lang);
		if (nmb < sizeof(path)) {
			ret = usbg_check_dir(path);
			if (ret == USBG_SUCCESS)
				ret = usbg_write_string(path, "", "configuration", str);
		} else {
			ret = USBG_ERROR_PATH_TOO_LONG;
		}
	}

	return ret;
}

int usbg_add_config_function(usbg_config *c, const char *name, usbg_function *f)
{
	char bpath[USBG_MAX_PATH_LENGTH];
	char fpath[USBG_MAX_PATH_LENGTH];
	usbg_binding *b;
	int ret = USBG_SUCCESS;
	int nmb;

	if (!c || !f) {
		ret = USBG_ERROR_INVALID_PARAM;
		goto out;
	}

	if (!name)
		name = f->name;

	b = usbg_get_binding(c, name);
	if (b) {
		ERROR("duplicate binding name\n");
		ret = USBG_ERROR_EXIST;
		goto out;
	}

	b = usbg_get_link_binding(c, f);
	if (b) {
		ERROR("duplicate binding link\n");
		ret = USBG_ERROR_EXIST;
		goto out;
	}

	nmb = snprintf(fpath, sizeof(fpath), "%s/%s", f->path, f->name);
	if (nmb >= sizeof(fpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	nmb = snprintf(bpath, sizeof(bpath), "%s/%s", c->path, c->name);
	if (nmb >= sizeof(bpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	b = usbg_allocate_binding(bpath, name, c);
	if (b) {
		int free_space = sizeof(bpath) - nmb;

		b->target = f;
		nmb = snprintf(&(bpath[nmb]), free_space, "/%s", name);
		if (nmb < free_space) {

			ret = symlink(fpath, bpath);
			if (ret == 0) {
				b->target = f;
				INSERT_TAILQ_STRING_ORDER(&c->bindings, bhead,
						name, b, bnode);
			} else {
				ERRORNO("%s -> %s\n", bpath, fpath);
				ret = usbg_translate_error(errno);
			}
		} else {
			ret = USBG_ERROR_PATH_TOO_LONG;
		}

		if (ret != USBG_SUCCESS) {
			usbg_free_binding(b);
			b = NULL;
		}
	} else {
		ret = USBG_ERROR_NO_MEM;
	}

out:
	return ret;
}

usbg_function *usbg_get_binding_target(usbg_binding *b)
{
	return b ? b->target : NULL;
}

const char *usbg_get_binding_name(usbg_binding *b)
{
	return b ? b->name : NULL;
}

size_t usbg_get_binding_name_len(usbg_binding *b)
{
	return b ? strlen(b->name) : USBG_ERROR_INVALID_PARAM;
}

int usbg_cpy_binding_name(usbg_binding *b, char *buf, size_t len)
{
	if (!b || !buf || len == 0)
		return USBG_ERROR_INVALID_PARAM;

	buf[--len] = '\0';
	strncpy(buf, b->name, len);

	return USBG_SUCCESS;
}

int usbg_enable_gadget(usbg_gadget *g, usbg_udc *udc)
{
	int ret = USBG_ERROR_INVALID_PARAM;

	if (!g)
		return ret;

	if (!udc) {
		udc = usbg_get_first_udc(g->parent);
		if (!udc)
			return ret;
	}

	ret = usbg_write_string(g->path, g->name, "UDC", udc->name);
	if (ret == USBG_SUCCESS) {
		/* If gadget has been detached and we didn't noticed
		 * it we have to clean up now.
		 */
		if (g->udc)
			g->udc->gadget = NULL;
		g->udc = udc;
		udc->gadget = g;
	}

	return ret;
}

int usbg_disable_gadget(usbg_gadget *g)
{
	int ret = USBG_ERROR_INVALID_PARAM;

	if (!g)
		return ret;

	ret = usbg_write_string(g->path, g->name, "UDC", "\n");
	if (ret == USBG_SUCCESS) {
		if (g->udc)
			g->udc->gadget = NULL;
		g->udc = NULL;
	}

	return ret;
}

/*
 * USB function-specific attribute configuration
 */

usbg_function_type usbg_get_function_type(usbg_function *f)
{
	return f ? f->type : USBG_ERROR_INVALID_PARAM;
}

int usbg_get_function_attrs(usbg_function *f, usbg_function_attrs *f_attrs)
{
	return f && f_attrs ? f->ops->get_attrs(f, f_attrs)
			: USBG_ERROR_INVALID_PARAM;
}

static void usbg_cleanup_function_ms_lun_attrs(usbg_f_ms_lun_attrs *lun_attrs)
{
	if (!lun_attrs)
		return;

	free((char*)lun_attrs->filename);
	lun_attrs->id = -1;
}

void usbg_cleanup_function_attrs(usbg_function_attrs *f_attrs)
{
	usbg_f_attrs *attrs;

	if (!f_attrs)
		return;

	attrs = &f_attrs->attrs;

	switch (f_attrs->header.attrs_type) {
	case USBG_F_ATTRS_SERIAL:
		break;

	case USBG_F_ATTRS_NET:
		free((char*)attrs->net.ifname);
		attrs->net.ifname = NULL;
		break;

	case USBG_F_ATTRS_PHONET:
		free((char*)attrs->phonet.ifname);
		attrs->phonet.ifname = NULL;
		break;

	case USBG_F_ATTRS_FFS:
		free((char*)attrs->ffs.dev_name);
		attrs->ffs.dev_name = NULL;
		break;

	case USBG_F_ATTRS_MS:
	{
		int i;
		usbg_f_ms_attrs *ms_attrs = &attrs->ms;

		if (!ms_attrs->luns)
			goto ms_break;

		for (i = 0; i < ms_attrs->nluns; ++i) {
			if (!ms_attrs->luns[i])
				continue;

			usbg_cleanup_function_ms_lun_attrs(ms_attrs->luns[i]);
			free(ms_attrs->luns[i]);
		}
		free(ms_attrs->luns);
		ms_attrs->luns = NULL;
		ms_attrs->nluns = -1;
	ms_break:
		break;
	}

	case USBG_F_ATTRS_MIDI:
		free((char*)attrs->midi.id);
		attrs->midi.id = NULL;
		break;

	case USBG_F_ATTRS_LOOPBACK:
		break;

	default:
		ERROR("Unsupported attrs type\n");
		break;
	}
}

int usbg_set_function_attrs(usbg_function *f,
			    const usbg_function_attrs *f_attrs)
{
	int ret = USBG_ERROR_INVALID_PARAM;

	if (!f || !f_attrs)
		return ret;

	ret = f->ops->set_attrs(f, f_attrs);
out:
	return ret;
}

int usbg_set_net_dev_addr(usbg_function *f, struct ether_addr *dev_addr)
{
	int ret = USBG_SUCCESS;

	if (f && dev_addr) {
		char str_buf[USBG_MAX_STR_LENGTH];
		char *str_addr = usbg_ether_ntoa_r(dev_addr, str_buf);
		ret = usbg_write_string(f->path, f->name, "dev_addr", str_addr);
	} else {
		ret = USBG_ERROR_INVALID_PARAM;
	}

	return ret;
}

int usbg_set_net_host_addr(usbg_function *f, struct ether_addr *host_addr)
{
	int ret = USBG_SUCCESS;

	if (f && host_addr) {
		char str_buf[USBG_MAX_STR_LENGTH];
		char *str_addr = usbg_ether_ntoa_r(host_addr, str_buf);
		ret = usbg_write_string(f->path, f->name, "host_addr", str_addr);
	} else {
		ret = USBG_ERROR_INVALID_PARAM;
	}

	return ret;
}

int usbg_set_net_qmult(usbg_function *f, int qmult)
{
	return f ? usbg_write_dec(f->path, f->name, "qmult", qmult)
			: USBG_ERROR_INVALID_PARAM;
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

usbg_udc *usbg_get_first_udc(usbg_state *s)
{
	return s ? TAILQ_FIRST(&s->udcs) : NULL;
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

usbg_udc *usbg_get_next_udc(usbg_udc *u)
{
	return u ? TAILQ_NEXT(u, unode) : NULL;
}

