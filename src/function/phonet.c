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
#include "usbg/function/phonet.h"

#include <malloc.h>
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

struct usbg_f_phonet {
	struct usbg_function func;
};

GENERIC_ALLOC_INST(phonet, struct usbg_f_phonet, func)

GENERIC_FREE_INST(phonet, struct usbg_f_phonet, func)

static int phonet_set_attrs(struct usbg_function *f, void *f_attrs)
{
	const char *ifname = *(const char **)f_attrs;

	return ifname && ifname[0] ? USBG_ERROR_INVALID_PARAM : USBG_SUCCESS;
}

static int phonet_get_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_phonet_get_ifname(usbg_to_phonet_function(f), f_attrs);
}

static void phonet_cleanup_attrs(struct usbg_function *f, void *f_attrs)
{
	free(*(char **)f_attrs);
}

static int phonet_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	return USBG_SUCCESS;
}

static int phonet_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	return USBG_SUCCESS;
}

struct usbg_function_type usbg_f_type_phonet = {
	.name = "phonet",
	.alloc_inst = phonet_alloc_inst,
	.free_inst = phonet_free_inst,
	.set_attrs = phonet_set_attrs,
	.get_attrs = phonet_get_attrs,
	.cleanup_attrs = phonet_cleanup_attrs,
	.import = phonet_libconfig_import,
	.export = phonet_libconfig_export,
};

/* API implementation */

usbg_f_phonet *usbg_to_phonet_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_phonet ?
		container_of(f, struct usbg_f_phonet, func) : NULL;
}

usbg_function *usbg_from_phonet_function(usbg_f_phonet *pf)
{
	return &pf->func;
}

int usbg_f_phonet_get_ifname(usbg_f_phonet *pf, char **ifname)
{
	struct usbg_function *f = &pf->func;

	if (!pf || !ifname)
		return USBG_ERROR_INVALID_PARAM;

	return usbg_read_string_alloc(f->path, f->name, "ifname", ifname);
}

int usbg_f_phonet_get_ifname_s(usbg_f_phonet *pf, char *buf, int len)
{
	struct usbg_function *f = &pf->func;
	int ret;

	if (!pf || !buf)
		return USBG_ERROR_INVALID_PARAM;

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
