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

#include <malloc.h>
#ifdef HAS_LIBCONFIG
#include <libconfig.h>
#endif

static int ether_set_attrs(struct usbg_function *f,
			   const usbg_function_attrs *f_attrs)
{
	int ret = USBG_SUCCESS;
	char addr_buf[USBG_MAX_STR_LENGTH];
	const usbg_f_net_attrs *attrs = &f_attrs->attrs.net;
	char *addr;

	/* ifname is read only so we accept only empty string for this param */
	if (attrs->ifname && attrs->ifname[0]) {
		ret = USBG_ERROR_INVALID_PARAM;
		goto out;
	}

	addr = usbg_ether_ntoa_r(&attrs->dev_addr, addr_buf);
	ret = usbg_write_string(f->path, f->name, "dev_addr", addr);
	if (ret != USBG_SUCCESS)
		goto out;

	addr = usbg_ether_ntoa_r(&attrs->host_addr, addr_buf);
	ret = usbg_write_string(f->path, f->name, "host_addr", addr);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_dec(f->path, f->name, "qmult", attrs->qmult);

out:
	return ret;
}

static int ether_get_attrs(struct usbg_function *f,
			   usbg_function_attrs *f_attrs)
{
	struct ether_addr *addr;
	struct ether_addr addr_buf;
	char str_addr[USBG_MAX_STR_LENGTH];
	usbg_f_net_attrs *attrs = &f_attrs->attrs.net;
	int ret = USBG_ERROR_INVALID_PARAM;

	ret = usbg_read_string(f->path, f->name, "dev_addr", str_addr);
	if (ret != USBG_SUCCESS)
		goto out;

	addr = ether_aton_r(str_addr, &addr_buf);
	if (addr) {
		attrs->dev_addr = *addr;
	} else {
		ret = USBG_ERROR_IO;
		goto out;
	}

	ret = usbg_read_string(f->path, f->name, "host_addr", str_addr);
	if (ret != USBG_SUCCESS)
		goto out;

	addr = ether_aton_r(str_addr, &addr_buf);
	if (addr) {
		attrs->host_addr = *addr;
	} else {
		ret = USBG_ERROR_IO;
		goto out;
	}

	ret = usbg_read_dec(f->path, f->name, "qmult", &(attrs->qmult));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_string_alloc(f->path, f->name, "ifname",
				     &(attrs->ifname));

	f_attrs->header.attrs_type = USBG_F_ATTRS_NET;
out:
	return ret;
}

static void ether_cleanup_attrs(struct usbg_function *f,
				usbg_function_attrs *f_attrs)
{
	free((char*)f_attrs->attrs.net.ifname);
	f_attrs->attrs.net.ifname = NULL;
}

#ifdef HAS_LIBCONFIG

static int ether_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	config_setting_t *node;
	int ret = USBG_SUCCESS;
	int qmult;
	struct ether_addr *addr;
	struct ether_addr addr_buf;
	const char *str;

#define GET_OPTIONAL_ADDR(NAME)					\
	do {							\
		node = config_setting_get_member(root, #NAME);	\
		if (node) {					\
			str = config_setting_get_string(node);	\
			if (!str) {				\
				ret = USBG_ERROR_INVALID_TYPE;	\
				goto out;			\
			}					\
								\
			addr = ether_aton_r(str, &addr_buf);	\
			if (!addr) {				\
				ret = USBG_ERROR_INVALID_VALUE;	\
				goto out;			\
			}					\
			ret = usbg_set_net_##NAME(f, addr);	\
			if (ret != USBG_SUCCESS)		\
				goto out;			\
		}						\
	} while (0)

	GET_OPTIONAL_ADDR(host_addr);
	GET_OPTIONAL_ADDR(dev_addr);

#undef GET_OPTIONAL_ADDR

	node = config_setting_get_member(root, "qmult");
	if (node) {
		if (!usbg_config_is_int(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto out;
		}
		qmult = config_setting_get_int(node);
		ret = usbg_set_net_qmult(f, qmult);
	}

out:
	return ret;
}

static int ether_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	config_setting_t *node;
	char *addr;
	char addr_buf[USBG_MAX_STR_LENGTH];
	int cfg_ret, usbg_ret;
	usbg_function_attrs f_attrs;
	usbg_f_net_attrs *attrs = &f_attrs.attrs.net;
	int ret = USBG_ERROR_NO_MEM;

	usbg_ret = ether_get_attrs(f, &f_attrs);
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, "dev_addr", CONFIG_TYPE_STRING);
	if (!node)
		goto cleanup;

	addr = usbg_ether_ntoa_r(&attrs->dev_addr, addr_buf);
	cfg_ret = config_setting_set_string(node, addr);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto cleanup;
	}

	node = config_setting_add(root, "host_addr", CONFIG_TYPE_STRING);
	if (!node)
		goto cleanup;

	addr = usbg_ether_ntoa_r(&attrs->host_addr, addr_buf);
	cfg_ret = config_setting_set_string(node, addr);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto cleanup;
	}

	node = config_setting_add(root, "qmult", CONFIG_TYPE_INT);
	if (!node)
		goto cleanup;

	cfg_ret = config_setting_set_int(node, attrs->qmult);
	ret = cfg_ret == CONFIG_TRUE ? 0 : USBG_ERROR_OTHER_ERROR;

	/* ifname is read only so we don't export it */
cleanup:
	ether_cleanup_attrs(f, &f_attrs);
out:
	return ret;
}

#define ETHER_LIBCONFIG_DEP_OPS			\
	.import = ether_libconfig_import,	\
	.export = ether_libconfig_export

#else

#define ETHER_LIBCONFIG_DEP_OPS

#endif /* HAS_LIBCONFIG */

#define ETHER_FUNCTION_OPTS			\
	.set_attrs = ether_set_attrs,		\
	.get_attrs = ether_get_attrs,		\
	.cleanup_attrs = ether_cleanup_attrs,	\
	ETHER_LIBCONFIG_DEP_OPS

struct usbg_function_type usbg_f_type_ecm = {
	.name = "ecm",
	ETHER_FUNCTION_OPTS
};

struct usbg_function_type usbg_f_type_subset = {
	.name = "geth",
	ETHER_FUNCTION_OPTS
};

struct usbg_function_type usbg_f_type_ncm = {
	.name = "ncm",
	ETHER_FUNCTION_OPTS
};

struct usbg_function_type usbg_f_type_eem = {
	.name = "eem",
	ETHER_FUNCTION_OPTS
};

struct usbg_function_type usbg_f_type_rndis = {
	.name = "rndis",
	ETHER_FUNCTION_OPTS
};

