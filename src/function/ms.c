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
#include "usbg/function/ms.h"

#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

#define MAX_LUNS 16

struct usbg_f_ms {
	struct usbg_function func;
	bool luns[MAX_LUNS];
	bool luns_initiated;
};

#define MS_LUN_BOOL_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_ms_lun_attrs, _name),     \
		.get = usbg_get_bool,				        \
		.set = usbg_set_bool,				        \
		.import = usbg_get_config_node_bool,	                \
		.export = usbg_set_config_node_bool,		        \
	}

#define MS_LUN_STRING_ATTR(_name)					\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_ms_lun_attrs, _name),     \
		.get = usbg_get_string,				        \
		.set = usbg_set_string,				        \
		.export = usbg_set_config_node_string,		        \
		.import = usbg_get_config_node_string,	                \
	}

static struct {
	const char *name;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} ms_lun_attr[USBG_F_MS_LUN_ATTR_MAX] = {
	[USBG_F_MS_LUN_CDROM] = MS_LUN_BOOL_ATTR(cdrom),
	[USBG_F_MS_LUN_RO] = MS_LUN_BOOL_ATTR(ro),
	[USBG_F_MS_LUN_NOFUA] = MS_LUN_BOOL_ATTR(nofua),
	[USBG_F_MS_LUN_REMOVABLE] = MS_LUN_BOOL_ATTR(removable),
	[USBG_F_MS_LUN_FILE] = MS_LUN_STRING_ATTR(file),
};

static inline int lun_select(const struct dirent *dent)
{
	int ret;
	int id;

	ret = file_select(dent);
	if (!ret)
		goto out;

	ret = sscanf(dent->d_name, "lun.%d", &id);
out:
	return ret;
}

static inline int lun_sort(const struct dirent **d1, const struct dirent **d2)
{
	int ret;
	int id1, id2;

	ret = sscanf((*d1)->d_name, "lun.%d", &id1);
	if (ret != 1)
		goto err;

	ret = sscanf((*d2)->d_name, "lun.%d", &id2);
	if (ret != 1)
		goto err;

	if (id1 < id2)
		ret = 1;

	return id1 < id2 ? -1 : id1 > id2;
err:
	/*
	 * This should not happened because dentries has been
	 * already checked by lun_select function. This
	 * error procedure is just in case.
	 */
	return -1;
}

int init_luns(struct usbg_f_ms *ms)
{
	struct dirent **dent;
	char lpath[USBG_MAX_PATH_LENGTH];
	int nmb, i, id;
	int ret = 0;

	ret = snprintf(lpath, sizeof(lpath), "%s/%s/",
		       ms->func.path, ms->func.name);
	if (ret >= sizeof(lpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		return ret;
	}

	nmb = scandir(lpath, &dent, lun_select, lun_sort);
	if (nmb < 0) {
		ret = usbg_translate_error(errno);
		return ret;
	}

	for (i = 0; i < nmb; ++i) {
		/* don't check the error as we know that name is valid */
		sscanf(dent[i]->d_name, "lun.%d", &id);
		ms->luns[id] = true;
		free(dent[i]);
	}
	free(dent);

	return 0;
}

static inline bool *get_lun_mask(struct usbg_f_ms *ms)
{
	if (!ms->luns_initiated) {
		int ret = init_luns(ms);
		if (!ret)
			ms->luns_initiated = true;
	}

	return ms->luns;
}

GENERIC_ALLOC_INST(ms_internal, struct usbg_f_ms, func);
static int ms_alloc_inst(struct usbg_function_type *type,
			 usbg_function_type type_code,
			 const char *instance, const char *path,
			 struct usbg_gadget *parent,
			 struct usbg_function **f)
{
	struct usbg_f_ms *mf;
	int ret;

	ret = ms_internal_alloc_inst(type, type_code, instance,
				     path, parent, f);
	if (ret)
		goto out;

	mf = usbg_to_ms_function(*f);
	memset(mf->luns, 0, sizeof(mf->luns));
	mf->luns_initiated = false;
out:
	return ret;
}

GENERIC_FREE_INST(ms, struct usbg_f_ms, func);

static int ms_set_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_ms_set_attrs(usbg_to_ms_function(f),
				   (struct usbg_f_ms_attrs *)f_attrs);
}

static void ms_cleanup_attrs(struct usbg_function *f, void *f_attrs)
{
	usbg_f_ms_cleanup_attrs((struct usbg_f_ms_attrs *)f_attrs);
}

static int ms_get_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_ms_get_attrs(usbg_to_ms_function(f),
				  (struct usbg_f_ms_attrs *)f_attrs);
}

#ifdef HAS_GADGET_SCHEMES

static int ms_import_lun_attrs(struct usbg_f_ms *mf, int lun_id,
			       config_setting_t *root)
{
	union usbg_f_ms_lun_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_MS_LUN_ATTR_MIN; i < USBG_F_MS_LUN_ATTR_MAX; ++i) {
		ret = ms_lun_attr[i].import(root, ms_lun_attr[i].name, &val);
		/* node not  found */
		if (ret == 0)
			continue;
		/* error */
		if (ret < 0)
			break;

		ret = usbg_f_ms_set_lun_attr_val(mf, lun_id, i, &val);
		if (ret)
			break;
	}

	return ret;
}

static int ms_libconfig_import(struct usbg_function *f, config_setting_t *root)
{
	struct usbg_f_ms *mf = usbg_to_ms_function(f);
	config_setting_t *luns_node, *node;
	bool *luns;
	bool stall;
	int i, nluns;
	int ret;

	ret = usbg_get_config_node_bool(root, "stall", &stall);
	if (ret > 0)
		ret = usbg_f_ms_set_stall(mf, stall);

	if (ret < 0)
		goto out;

	luns_node = config_setting_get_member(root, "luns");
	if (!luns_node) {
		ret = USBG_ERROR_INVALID_PARAM;
		goto out;
	}

	if (!config_setting_is_list(luns_node)) {
		ret = USBG_ERROR_INVALID_TYPE;
		goto out;
	}

	nluns = config_setting_length(luns_node);
	luns = get_lun_mask(mf);
	for (i = 0; i < nluns; ++i) {
		node = config_setting_get_elem(luns_node, i);
		if (!node) {
			ret = USBG_ERROR_INVALID_FORMAT;
			goto out;
		}

		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto out;
		}
		if (!luns[i]) {
			ret = usbg_f_ms_create_lun(mf, i, NULL);
			if (ret)
				goto out;
		}

		ret = ms_import_lun_attrs(mf, i, node);
		if (ret)
			goto out;
	}

	ret = 0;
out:
	return ret;
}

static int ms_export_lun_attrs(struct usbg_f_ms *mf, int lun_id,
				      config_setting_t *root)
{
	union usbg_f_ms_lun_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_MS_LUN_ATTR_MIN; i < USBG_F_MS_LUN_ATTR_MAX; ++i) {
		ret = usbg_f_ms_get_lun_attr_val(mf, lun_id, i, &val);
		if (ret)
			break;

		ret = ms_lun_attr[i].export(root, ms_lun_attr[i].name, &val);
		if (ret)
			break;
	}

	return ret;
}

static int ms_libconfig_export(struct usbg_function *f, config_setting_t *root)
{
	struct usbg_f_ms *mf = usbg_to_ms_function(f);
	bool *luns;
	bool stall;
	int i;
	config_setting_t *luns_node, *node;
	int ret;

	ret = usbg_f_ms_get_stall(mf, &stall);
	if (ret)
		goto out;

	ret = usbg_set_config_node_bool(root, "stall", &stall);
	if (ret)
		goto out;

	luns_node = config_setting_add(root, "luns", CONFIG_TYPE_LIST);
	if (!luns_node)
		goto out;

	luns = get_lun_mask(mf);
	for (i = 0; i < MAX_LUNS; ++i) {
		if (!luns[i])
			continue;

		node = config_setting_add(luns_node, "", CONFIG_TYPE_GROUP);
		if (!node)
			goto out;

		ret = ms_export_lun_attrs(mf, i, node);
		if (ret)
			goto out;
	}
out:
	return ret;
}

#endif /* HAS_GADGET_SCHEMES */

static int ms_remove(struct usbg_function *f, int opts)
{
	struct usbg_f_ms *mf = usbg_to_ms_function(f);
	bool *luns;
	int i;
	int ret = USBG_SUCCESS;

	luns = get_lun_mask(mf);
	/* LUN0 cannot be removed */
	for (i = 1; i < MAX_LUNS; ++i) {
		if (!luns[i])
			continue;

		ret = usbg_f_ms_rm_lun(mf, i);
		if (ret)
			break;
	}

	return ret;
}

struct usbg_function_type usbg_f_type_ms = {
	.name = "mass_storage",
	.alloc_inst = ms_alloc_inst,
	.free_inst = ms_free_inst,
	.set_attrs = ms_set_attrs,
	.get_attrs = ms_get_attrs,
	.cleanup_attrs = ms_cleanup_attrs,

#ifdef HAS_GADGET_SCHEMES
	.import = ms_libconfig_import,
	.export = ms_libconfig_export,
#endif

	.remove = ms_remove,
};

/* API implementation */

usbg_f_ms *usbg_to_ms_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_ms ?
		container_of(f, struct usbg_f_ms, func) : NULL;
}

usbg_function *usbg_from_ms_function(usbg_f_ms *mf)
{
	return &mf->func;
}

int usbg_f_ms_get_attrs(usbg_f_ms *mf,
			  struct usbg_f_ms_attrs *attrs)
{
	int i;
	const bool *luns;
	struct usbg_f_ms_lun_attrs **lun_attrs;
	int nluns;
	int ret;

	ret = usbg_f_ms_get_stall(mf, &attrs->stall);
	if (ret)
		goto out;

	luns = get_lun_mask(mf);
	usbg_f_ms_get_nluns(mf, &nluns);

	lun_attrs = calloc(nluns + 1, sizeof(*lun_attrs));
	if (!lun_attrs)
		return USBG_ERROR_NO_MEM;

	for (i = 0; i < MAX_LUNS; ++i) {
		if (!luns[i])
			continue;

		lun_attrs[i] = malloc(sizeof(*lun_attrs[i]));
		if (!lun_attrs[i]) {
			ret = USBG_ERROR_NO_MEM;
			goto free_lun_attrs;
		}

		ret = usbg_f_ms_get_lun_attrs(mf, i, lun_attrs[i]);
		if (ret)
			goto free_current_lun_attrs;
	}

	attrs->nluns = nluns;
	attrs->luns = lun_attrs;

	return 0;

free_current_lun_attrs:
	free(lun_attrs[i]);
	lun_attrs[i] = NULL;

free_lun_attrs:
	for (i = 0; i < nluns; ++i) {
		usbg_f_ms_cleanup_lun_attrs(lun_attrs[i]);
		free(lun_attrs[i]);
	}

	free(lun_attrs);
out:
	return ret;
}

int usbg_f_ms_set_attrs(usbg_f_ms *mf,
			 const struct usbg_f_ms_attrs *attrs)
{
	int i, j;
	const bool *luns;
	int luns_to_be[MAX_LUNS];
	struct usbg_f_ms_lun_attrs **lun_attrs;
	int nluns;
	int ret;

	ret = usbg_f_ms_set_stall(mf, attrs->stall);
	if (ret)
		goto out;

	luns = get_lun_mask(mf);
	usbg_f_ms_get_nluns(mf, &nluns);

	for (i = 0; i < MAX_LUNS; ++i)
		luns_to_be[i] = -1;

	for (lun_attrs = attrs->luns, i = 0; lun_attrs[i]; ++i) {
		int id = lun_attrs[i]->id;

		id = id >= 0 ? id : i;
		luns_to_be[id] = i;
	}

	for (i = 0; i < MAX_LUNS; ++i) {
		if (luns[i]) {
			/* LUN exist and should exist so we set attr only */
			if (luns_to_be[i] >= 0)
				ret = usbg_f_ms_set_lun_attrs(mf, i,
							      attrs->luns[i]);
			/* LUN exist but should not so let's remove it */
			else
				ret = usbg_f_ms_rm_lun(mf, i);
		} else if (luns_to_be[i] >= 0) {
			ret = usbg_f_ms_create_lun(mf, i, attrs->luns[i]);
		}

		if (ret)
			goto cleanup_luns;
	}

	return 0;

cleanup_luns:
	for (j = 0; j < i ; ++j)
		if (!luns[j] && luns_to_be[j] >= 0)
			usbg_f_ms_rm_lun(mf, j);

out:
	return ret;
}

int usbg_f_ms_get_stall(usbg_f_ms *mf, bool *stall)
{
	return usbg_read_bool(mf->func.path, mf->func.name, "stall", stall);
}

int usbg_f_ms_set_stall(usbg_f_ms *mf, bool stall)
{
	return usbg_write_bool(mf->func.path, mf->func.name, "stall", stall);
}

int usbg_f_ms_get_nluns(usbg_f_ms *mf, int *nluns)
{
	int i;
	const bool *luns;

	*nluns = 0;
	for (luns = get_lun_mask(mf), i = 0; i < MAX_LUNS; ++i)
		*nluns += luns[i] ? 1 : 0;

	return 0;
}

int usbg_f_ms_create_lun(usbg_f_ms *mf, int lun_id,
			 struct usbg_f_ms_lun_attrs *lattrs)
{
	int ret;
	bool *luns;
	char lpath[USBG_MAX_PATH_LENGTH];

	if (lun_id >= MAX_LUNS)
		return USBG_ERROR_INVALID_PARAM;

	luns = get_lun_mask(mf);
	if (luns[lun_id])
		return USBG_ERROR_EXIST;

	ret = snprintf(lpath, sizeof(lpath), "%s/%s/lun.%d/",
		       mf->func.path, mf->func.name, lun_id);
	if (ret >= sizeof(lpath))
		return USBG_ERROR_PATH_TOO_LONG;

	ret = mkdir(lpath, S_IRWXU|S_IRWXG|S_IRWXO);
	if (ret)
		return usbg_translate_error(errno);

	if (lattrs) {
		ret = usbg_f_ms_set_lun_attrs(mf, lun_id, lattrs);
		if (ret)
			goto remove_lun;
	}

	luns[lun_id] = true;
	return 0;

remove_lun:
	rmdir(lpath);
	return ret;
}

int usbg_f_ms_rm_lun(usbg_f_ms *mf, int lun_id)
{
	int ret;
	bool *luns;
	char lpath[USBG_MAX_PATH_LENGTH];

	if (lun_id >= MAX_LUNS)
		return USBG_ERROR_INVALID_PARAM;

	luns = get_lun_mask(mf);
	if (!luns[lun_id])
		return USBG_ERROR_INVALID_PARAM;

	ret = snprintf(lpath, sizeof(lpath), "%s/%s/lun.%d/",
		       mf->func.path, mf->func.name, lun_id);
	if (ret >= sizeof(lpath))
		return USBG_ERROR_PATH_TOO_LONG;

	ret = rmdir(lpath);
	if (ret)
		return usbg_translate_error(errno);

	luns[lun_id] = false;
	return ret;
}

int usbg_f_ms_get_lun_attrs(usbg_f_ms *mf, int lun_id,
			struct usbg_f_ms_lun_attrs *lattrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_MS_LUN_ATTR_MIN; i < USBG_F_MS_LUN_ATTR_MAX; ++i) {
		ret = usbg_f_ms_get_lun_attr_val(mf, lun_id, i,
					       (union usbg_f_ms_lun_attr_val *)
					       ((char *)lattrs
						+ ms_lun_attr[i].offset));
		if (ret)
			break;
	}

	lattrs->id = lun_id;

	return ret;

}

int usbg_f_ms_set_lun_attrs(usbg_f_ms *mf, int lun_id,
			    const struct usbg_f_ms_lun_attrs *lattrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_MS_LUN_ATTR_MIN; i < USBG_F_MS_LUN_ATTR_MAX; ++i) {
		ret = usbg_f_ms_set_lun_attr_val(mf, lun_id, i,
					       (const union usbg_f_ms_lun_attr_val *)
					       ((const char *)lattrs
						+ ms_lun_attr[i].offset));
		if (ret)
			break;
	}

	return ret;
}

int usbg_f_ms_get_lun_attr_val(usbg_f_ms *mf, int lun_id,
			       enum usbg_f_ms_lun_attr lattr,
			       union usbg_f_ms_lun_attr_val *val)
{
	char lpath[USBG_MAX_PATH_LENGTH];
	int ret;

	ret = snprintf(lpath, sizeof(lpath), "%s/%s/lun.%d/",
		       mf->func.path, mf->func.name, lun_id);
	if (ret >= sizeof(lpath))
		return USBG_ERROR_PATH_TOO_LONG;

	return ms_lun_attr[lattr].get(lpath, "",
				    ms_lun_attr[lattr].name, val);
}

int usbg_f_ms_set_lun_attr_val(usbg_f_ms *mf, int lun_id,
			       enum usbg_f_ms_lun_attr lattr,
			       const union usbg_f_ms_lun_attr_val *val)
{
	char lpath[USBG_MAX_PATH_LENGTH];
	int ret;

	ret = snprintf(lpath, sizeof(lpath), "%s/%s/lun.%d/",
		       mf->func.path, mf->func.name, lun_id);
	if (ret >= sizeof(lpath))
		return USBG_ERROR_PATH_TOO_LONG;

	return 	ms_lun_attr[lattr].set(lpath, "",
				       ms_lun_attr[lattr].name, val);
}

int usbg_f_ms_get_lun_file_s(usbg_f_ms *mf, int lun_id,
				 char *buf, int len)
{
	char lpath[USBG_MAX_PATH_LENGTH];
	int ret;

	if (!mf || !buf)
		return USBG_ERROR_INVALID_PARAM;

	ret = snprintf(lpath, sizeof(lpath), "%s/%s/lun.%d/",
		       mf->func.path, mf->func.name, lun_id);
	if (ret >= sizeof(lpath))
		return USBG_ERROR_PATH_TOO_LONG;
	/*
	 * TODO:
	 * Rework usbg_common to make this function consistent with doc.
	 * This below is only an ugly hack
	 */
	ret = usbg_read_string_limited(lpath, "", "filename", buf, len);
	if (ret)
		goto out;

	ret = strlen(buf);
out:
	return ret;
}
