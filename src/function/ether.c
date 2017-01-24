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
#include "usbg/function/net.h"

#include <malloc.h>
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

struct usbg_f_net {
	struct usbg_function func;
};

#define NET_DEC_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.ro = false,					        \
		.offset = offsetof(struct usbg_f_net_attrs, _name),     \
		.get = usbg_get_dec,				        \
		.set = usbg_set_dec,				        \
		.import = usbg_get_config_node_int,	                \
		.export = usbg_set_config_node_int,		        \
	}

#define NET_RO_STRING_ATTR(_name)					\
	{								\
		.name = #_name,						\
		.ro = true,					        \
		.offset = offsetof(struct usbg_f_net_attrs, _name),     \
		.get = usbg_get_string,				        \
		.export = usbg_set_config_node_string,		        \
	}

#define NET_ETHER_ADDR_ATTR(_name)					\
	{								\
		.name = #_name,						\
		.ro = false,					        \
		.offset = offsetof(struct usbg_f_net_attrs, _name),     \
		.get = usbg_get_ether_addr,			        \
		.set = usbg_set_ether_addr,			        \
		.import = usbg_get_config_node_ether_addr,	        \
		.export = usbg_set_config_node_ether_addr,	        \
	}

static struct {
	const char *name;
	bool ro;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} net_attr[USBG_F_NET_ATTR_MAX] = {
	[USBG_F_NET_DEV_ADDR] = NET_ETHER_ADDR_ATTR(dev_addr),
	[USBG_F_NET_HOST_ADDR] = NET_ETHER_ADDR_ATTR(host_addr),
	[USBG_F_NET_IFNAME] = NET_RO_STRING_ATTR(ifname),
	[USBG_F_NET_QMULT] = NET_DEC_ATTR(qmult),
};

#undef NET_DEC_ATTR
#undef NET_STRING_ATTR

GENERIC_ALLOC_INST(ether, struct usbg_f_net, func);

GENERIC_FREE_INST(ether, struct usbg_f_net, func);

static int ether_set_attrs(struct usbg_function *f, void *f_attrs)
{
	const struct usbg_f_net_attrs *attrs = f_attrs;

	/* ifname is read only so we accept only empty string for this param */
	if (attrs->ifname && attrs->ifname[0])
		return USBG_ERROR_INVALID_PARAM;

	return usbg_f_net_set_attrs(usbg_to_net_function(f), attrs);
}

static int ether_get_attrs(struct usbg_function *f, void *f_attrs)
{
	struct usbg_f_net_attrs *attrs = f_attrs;

	return usbg_f_net_get_attrs(usbg_to_net_function(f), attrs);
}

static void ether_cleanup_attrs(struct usbg_function *f, void *f_attrs)
{
	usbg_f_net_cleanup_attrs(f_attrs);
}

#ifdef HAS_GADGET_SCHEMES

static int ether_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_net *nf = usbg_to_net_function(f);
	union usbg_f_net_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_NET_ATTR_MIN; i < USBG_F_NET_ATTR_MAX; ++i) {
		if (net_attr[i].ro)
			continue;

		ret = net_attr[i].import(root, net_attr[i].name, &val);
		/* node not  found */
		if (ret == 0)
			continue;
		/* error */
		if (ret < 0)
			break;

		ret = usbg_f_net_set_attr_val(nf, i, val);
		if (ret)
			break;
	}

	return ret;
}

static int ether_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_net *nf = usbg_to_net_function(f);
	union usbg_f_net_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_NET_ATTR_MIN; i < USBG_F_NET_ATTR_MAX; ++i) {
		if (net_attr[i].ro)
			continue;

		ret = usbg_f_net_get_attr_val(nf, i, &val);
		if (ret)
			break;

		ret = net_attr[i].export(root, net_attr[i].name, &val);
		if (ret)
			break;
	}

	return ret;
}

#define ETHER_LIBCONFIG_DEP_OPS			\
	.import = ether_libconfig_import,	\
	.export = ether_libconfig_export

#else

#define ETHER_LIBCONFIG_DEP_OPS

#endif /* HAS_GADGET_SCHEMES */

#define ETHER_FUNCTION_OPTS			\
	.alloc_inst = ether_alloc_inst,		\
	.free_inst = ether_free_inst,		\
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

static char *rndis_os_desc_ifnames[] = {
	"rndis",
	NULL
};

struct usbg_function_type usbg_f_type_rndis = {
	.name = "rndis",
	.os_desc_iname = rndis_os_desc_ifnames,
	ETHER_FUNCTION_OPTS
};

/* API implementation */

usbg_f_net *usbg_to_net_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_ecm
		|| f->ops == &usbg_f_type_subset
		|| f->ops == &usbg_f_type_ncm
		|| f->ops == &usbg_f_type_eem
		|| f->ops == &usbg_f_type_rndis ?
		container_of(f, struct usbg_f_net, func) : NULL;
}

usbg_function *usbg_from_net_function(usbg_f_net *nf)
{
	return &nf->func;
}

int usbg_f_net_get_attrs(usbg_f_net *nf,
			  struct usbg_f_net_attrs *attrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_NET_ATTR_MIN; i < USBG_F_NET_ATTR_MAX; ++i) {
		ret = usbg_f_net_get_attr_val(nf, i,
					       (union usbg_f_net_attr_val *)
					       ((char *)attrs
						+ net_attr[i].offset));
		if (ret)
			break;
	}

	return ret;

}

int usbg_f_net_set_attrs(usbg_f_net *nf,
			 const struct usbg_f_net_attrs *attrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_NET_ATTR_MIN; i < USBG_F_NET_ATTR_MAX; ++i) {
		if (net_attr[i].ro)
			continue;

		ret = usbg_f_net_set_attr_val(nf, i,
					       *(union usbg_f_net_attr_val *)
					       ((char *)attrs
						+ net_attr[i].offset));
		if (ret)
			break;
	}

	return ret;

}

int usbg_f_net_get_attr_val(usbg_f_net *nf, enum usbg_f_net_attr attr,
			    union usbg_f_net_attr_val *val)
{
	return net_attr[attr].get(nf->func.path, nf->func.name,
				    net_attr[attr].name, val);
}

int usbg_f_net_set_attr_val(usbg_f_net *nf, enum usbg_f_net_attr attr,
			    union usbg_f_net_attr_val val)
{
	return net_attr[attr].ro ?
		USBG_ERROR_INVALID_PARAM :
		net_attr[attr].set(nf->func.path, nf->func.name,
				   net_attr[attr].name, &val);
}

int usbg_f_net_get_ifname_s(usbg_f_net *nf, char *buf, int len)
{
	struct usbg_function *f;
	int ret;

	if (!nf || !buf)
		return USBG_ERROR_INVALID_PARAM;

	f = &nf->func;
	/*
	 * TODO:
	 * Rework usbg_common to make this function consistent with doc.
	 * This below is only an ugly hack
	 */
	ret = usbg_read_string_limited(f->path, f->name, "ifname", buf, len);
	if (ret)
		goto out;

	ret = strlen(buf);
out:
	return ret;
}
