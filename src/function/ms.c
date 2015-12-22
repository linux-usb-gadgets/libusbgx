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
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAS_LIBCONFIG
#include <libconfig.h>
#endif

struct usbg_f_ms {
	struct usbg_function func;
};

GENERIC_ALLOC_INST(ms, struct usbg_f_ms, func);

GENERIC_FREE_INST(ms, struct usbg_f_ms, func);

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

static int ms_set_lun_attrs(const char *path, const char *lun,
				   usbg_f_ms_lun_attrs *lun_attrs)
{
	int ret;

	ret = usbg_write_bool(path, lun, "cdrom", lun_attrs->cdrom);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_bool(path, lun, "ro", lun_attrs->ro);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_bool(path, lun, "nofua", lun_attrs->nofua);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_bool(path, lun, "removable", lun_attrs->removable);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_string(path, lun, "file",
				      lun_attrs->filename);

out:
	return ret;
}

static int ms_set_attrs(struct usbg_function *f,
			const usbg_function_attrs *f_attrs)
{
	int i, nmb;
	int space_left;
	char *new_lun_mask;
	char lpath[USBG_MAX_PATH_LENGTH];
	char *lpath_end;
	DIR *dir;
	struct dirent **dent;
	const usbg_f_ms_attrs *attrs = &f_attrs->attrs.ms;
	int ret = USBG_ERROR_INVALID_PARAM;

	if (f_attrs->header.attrs_type &&
	    f_attrs->header.attrs_type != USBG_F_ATTRS_MS)
		goto out;

	ret = usbg_write_bool(f->path, f->name, "stall", attrs->stall);
	if (ret != USBG_SUCCESS)
		goto out;

	/* lun0 cannot be removed */
	if (!attrs->luns || attrs->nluns <= 0)
		goto out;

	ret = snprintf(lpath, sizeof(lpath), "%s/%s/", f->path, f->name);
	if (ret >= sizeof(lpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	lpath_end = lpath + strlen(lpath);
	space_left = sizeof(lpath) - (lpath_end - lpath);

	new_lun_mask = calloc(attrs->nluns, sizeof (char));
	if (!new_lun_mask) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	for (i = 0; i < attrs->nluns; ++i) {
		usbg_f_ms_lun_attrs *lun = attrs->luns[i];

		/*
		 * id may be left unset in lun attrs but
		 * if it is set it has to be equal to position
		 * in lun array
		 */
		if (lun && lun->id >= 0 && lun->id != i) {
			ret = USBG_ERROR_INVALID_PARAM;
			goto err_lun_loop;
		}

		ret = snprintf(lpath_end, space_left, "/lun.%d/", i);
		if (ret >= space_left) {
			ret = USBG_ERROR_PATH_TOO_LONG;
			goto err_lun_loop;
		}

		/*
		 * Check if dir exist and create it if needed
		 */
		dir = opendir(lpath);
		if (dir) {
			closedir(dir);
		} else if (errno != ENOENT) {
			ret = usbg_translate_error(errno);
			goto err_lun_loop;
		} else {
			ret = mkdir(lpath, S_IRWXU|S_IRWXG|S_IRWXO);
			if (!ret) {
				/*
				 * If we have created a new directory in
				 * this function let's mark it so we can
				 * cleanup in case of error
				 */
				new_lun_mask[i] = 1;
			} else {
				ret = usbg_translate_error(errno);
				goto err_lun_loop;
			}
		}

		/* if attributes has not been provided just go to next one */
		if (!lun)
			continue;

		ret = ms_set_lun_attrs(lpath, "", lun);
		if (ret != USBG_SUCCESS)
			goto err_lun_loop;
	}

	/* Check if function has more luns and remove them */
	*lpath_end = '\0';
	i = 0;
	nmb = scandir(lpath, &dent, lun_select, lun_sort);
	if (nmb < 0) {
		ret = usbg_translate_error(errno);
		goto err_lun_loop;
	}

	for (i = 0; i < attrs->nluns; ++i)
		free(dent[i]);

	for (; i < nmb; ++i) {
		ret = usbg_rm_dir(lpath, dent[i]->d_name);
		free(dent[i]);
		/* There is no good way to recover form this */
		if (ret != USBG_SUCCESS)
			goto err_rm_loop;
	}
	free(dent);

	return USBG_SUCCESS;

err_rm_loop:
	while (++i < nmb)
		free(dent[i]);
	free(dent);

	i = attrs->nluns;
err_lun_loop:
	/* array is null terminated so we may access lun[nluns] */
	for (; i >= 0; --i) {
		if (!new_lun_mask[i])
			continue;

		ret = snprintf(lpath_end, space_left, "/lun.%d/", i);
		if (ret >= space_left) {
			/*
			 * This should not happen because if we were
			 * able to create this directory we should be
			 * also able to remove it.
			 */
			continue;
		}
		rmdir(lpath);
	}
	free(new_lun_mask);

out:
	return ret;
}

static void ms_cleanup_lun_attrs(usbg_f_ms_lun_attrs *lun_attrs)
{
	if (!lun_attrs)
		return;

	free((char*)lun_attrs->filename);
	lun_attrs->id = -1;
}


static void ms_cleanup_attrs(struct usbg_function *f,
			     usbg_function_attrs *f_attrs)
{
	int i;
	usbg_f_ms_attrs *attrs = &f_attrs->attrs.ms;

	if (!attrs->luns)
		return;

	for (i = 0; i < attrs->nluns; ++i) {
		if (!attrs->luns[i])
			continue;

		ms_cleanup_lun_attrs(attrs->luns[i]);
		free(attrs->luns[i]);
	}
	free(attrs->luns);
	attrs->luns = NULL;
	attrs->nluns = -1;
}

static int ms_get_lun_attrs(const char *path, const char *lun,
					    usbg_f_ms_lun_attrs *lun_attrs)
{
	int ret;

	memset(lun_attrs, 0, sizeof(*lun_attrs));

	ret = sscanf(lun, "lun.%d", &lun_attrs->id);
	if (ret != 1)
		goto out;

	ret = usbg_read_bool(path, lun, "cdrom", &(lun_attrs->cdrom));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_bool(path, lun, "ro", &(lun_attrs->ro));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_bool(path, lun, "nofua", &(lun_attrs->nofua));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_bool(path, lun, "removable", &(lun_attrs->removable));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_string_alloc(path, lun, "file",
				     &(lun_attrs->filename));

out:
	return ret;
}

static int ms_get_attrs(struct usbg_function *f,
			usbg_function_attrs *f_attrs)
{
	int nmb;
	int i = 0;
	char fpath[USBG_MAX_PATH_LENGTH];
	usbg_f_ms_lun_attrs *lun_attrs;
	usbg_f_ms_lun_attrs **luns;
	struct dirent **dent;
	usbg_f_ms_attrs *attrs = &f_attrs->attrs.ms;
	int ret;

	ret = usbg_read_bool(f->path, f->name, "stall",
			     &(attrs->stall));
	if (ret != USBG_SUCCESS)
		goto out;


	nmb = snprintf(fpath, sizeof(fpath), "%s/%s/",
		       f->path, f->name);
	if (nmb >= sizeof(fpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	nmb = scandir(fpath, &dent, lun_select, lun_sort);
	if (nmb < 0) {
		ret = usbg_translate_error(errno);
		goto out;
	}

	luns = calloc(nmb + 1, sizeof(*luns));
	if (!luns) {
		ret = USBG_ERROR_NO_MEM;
		goto err;
	}

	attrs->luns = luns;
	attrs->nluns = nmb;

	for (i = 0; i < nmb; i++) {
		lun_attrs = malloc(sizeof(*lun_attrs));
		if (!lun_attrs) {
			ret = USBG_ERROR_NO_MEM;
			goto err;
		}

		ret = ms_get_lun_attrs(fpath, dent[i]->d_name,
						       lun_attrs);
		if (ret != USBG_SUCCESS) {
			free(lun_attrs);
			goto err;
		}

		luns[i] = lun_attrs;
		free(dent[i]);
	}
	free(dent);

	f_attrs->header.attrs_type = USBG_F_ATTRS_MS;

	return USBG_SUCCESS;

err:
	while (i < nmb) {
		free(dent[i]);
		++i;
	}
	free(dent);

	ms_cleanup_attrs(f, f_attrs);
out:
	return ret;
}

#ifdef HAS_LIBCONFIG

static int ms_import_lun_attrs(usbg_f_ms_lun_attrs *lattrs,
				      config_setting_t *root)
{
	config_setting_t *node;
	int i;
	int ret = USBG_ERROR_NO_MEM;

#define BOOL_ATTR(_name, _default_val) \
	{ .name = #_name, .value = &lattrs->_name, }
	struct {
		char *name;
		bool *value;
		bool default_val;
	} bool_attrs[] = {
		BOOL_ATTR(cdrom, false),
		BOOL_ATTR(ro, false),
		BOOL_ATTR(nofua, false),
		BOOL_ATTR(removable, true),
	};
#undef BOOL_ATTR

	memset(lattrs, 0, sizeof(*lattrs));
	lattrs->id = -1;

	for (i = 0; i < ARRAY_SIZE(bool_attrs); ++i) {
		*(bool_attrs[i].value) = bool_attrs[i].default_val;

		node = config_setting_get_member(root, bool_attrs[i].name);
		if (!node)
			continue;

		ret = config_setting_type(node);
		switch (ret) {
		case CONFIG_TYPE_INT:
			*(bool_attrs[i].value) = !!config_setting_get_int(node);
			break;
		case CONFIG_TYPE_BOOL:
			*(bool_attrs[i].value) = config_setting_get_bool(node);
			break;
		default:
			ret = USBG_ERROR_INVALID_TYPE;
			goto out;
		}
	}

	node = config_setting_get_member(root, "filename");
	if (node) {
		if (!usbg_config_is_string(node)) {
			ret = USBG_ERROR_INVALID_PARAM;
			goto out;
		}
		lattrs->filename = (char *)config_setting_get_string(node);
	} else {
			lattrs->filename = "";
	}

	ret = USBG_SUCCESS;
out:
	return ret;
}

static int ms_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	config_setting_t *luns_node, *node;
	int i;
	int ret = USBG_ERROR_NO_MEM;
	usbg_function_attrs f_attrs;
	usbg_f_ms_attrs *attrs = &f_attrs.attrs.ms;

	memset(&attrs, 0, sizeof(attrs));

	node = config_setting_get_member(root, "stall");
	if (node) {
		ret = config_setting_type(node);
		switch (ret) {
		case CONFIG_TYPE_INT:
			attrs->stall = !!config_setting_get_int(node);
			break;
		case CONFIG_TYPE_BOOL:
			attrs->stall = config_setting_get_bool(node);
			break;
		default:
			ret = USBG_ERROR_INVALID_TYPE;
			goto out;
		}
	}

	luns_node = config_setting_get_member(root, "luns");
	if (!node) {
		ret = USBG_ERROR_INVALID_PARAM;
		goto out;
	}

	if (!config_setting_is_list(luns_node)) {
		ret = USBG_ERROR_INVALID_TYPE;
		goto out;
	}

	attrs->nluns = config_setting_length(luns_node);

	attrs->luns = calloc(attrs->nluns + 1, sizeof(*(attrs->luns)));
	if (!attrs->luns) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	for (i = 0; i < attrs->nluns; ++i) {
		node = config_setting_get_elem(luns_node, i);
		if (!node) {
			ret = USBG_ERROR_INVALID_FORMAT;
			goto free_luns;
		}

		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto free_luns;
		}

		attrs->luns[i] = malloc(sizeof(*(attrs->luns[i])));
		if (!attrs->luns[i]) {
			ret = USBG_ERROR_NO_MEM;
			goto free_luns;
		}

		ret = ms_import_lun_attrs(attrs->luns[i], node);
		if (ret != USBG_SUCCESS)
			goto free_luns;
	}

	ret = usbg_set_function_attrs(f, &f_attrs);

free_luns:
	while (--i >= 0)
		if (attrs->luns[i])
			free(attrs->luns[i]);
	free(attrs->luns);
out:
	return ret;
}

static int ms_export_lun_attrs(usbg_f_ms_lun_attrs *lattrs,
				      config_setting_t *root)
{
	config_setting_t *node;
	int cfg_ret;
	int i;
	int ret = USBG_ERROR_NO_MEM;

#define BOOL_ATTR(_name) { .name = #_name, .value = &lattrs->_name, }
	struct {
		char *name;
		bool *value;
	} bool_attrs[] = {
		BOOL_ATTR(cdrom),
		BOOL_ATTR(ro),
		BOOL_ATTR(nofua),
		BOOL_ATTR(removable),
	};
#undef BOOL_ATTR

	for (i = 0; i < ARRAY_SIZE(bool_attrs); ++i) {
		node = config_setting_add(root, bool_attrs[i].name, CONFIG_TYPE_BOOL);
		if (!node)
			goto out;

		cfg_ret = config_setting_set_bool(node, *(bool_attrs[i].value));
		if (cfg_ret != CONFIG_TRUE) {
			ret = USBG_ERROR_OTHER_ERROR;
			goto out;
		}
	}

	node = config_setting_add(root, "filename", CONFIG_TYPE_STRING);
	if (!node)
		goto out;

	cfg_ret = config_setting_set_string(node, lattrs->filename);
	ret = cfg_ret == CONFIG_TRUE ? USBG_SUCCESS : USBG_ERROR_OTHER_ERROR;

out:
	return ret;
}

static int ms_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	config_setting_t *luns_node, *node;
	int i;
	int cfg_ret, usbg_ret;
	usbg_function_attrs f_attrs;
	usbg_f_ms_attrs *attrs = &f_attrs.attrs.ms;
	int ret = USBG_ERROR_NO_MEM;

	usbg_ret = ms_get_attrs(f, &f_attrs);
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

	node = config_setting_add(root, "stall", CONFIG_TYPE_BOOL);
	if (!node)
		goto cleanup;

	cfg_ret = config_setting_set_bool(node, attrs->stall);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto cleanup;
	}

	luns_node = config_setting_add(root, "luns", CONFIG_TYPE_LIST);
	if (!luns_node)
		goto cleanup;

	for (i = 0; i < attrs->nluns; ++i) {
		node = config_setting_add(luns_node, "", CONFIG_TYPE_GROUP);
		if (!node)
			goto cleanup;

		ret = ms_export_lun_attrs(attrs->luns[i], node);
		if (ret != USBG_SUCCESS)
			goto cleanup;
	}

	ret = USBG_SUCCESS;
cleanup:
	ms_cleanup_attrs(f, &f_attrs);
out:
	return ret;
}

#endif /* HAS_LIBCONFIG */

static int ms_remove(struct usbg_function *f, int opts)
{
	int ret;
	int nmb;
	int i;
	char lpath[USBG_MAX_PATH_LENGTH];
	struct dirent **dent;

	if (!(opts & USBG_RM_RECURSE))
		return 0;

	ret = snprintf(lpath, sizeof(lpath), "%s/%s/", f->path, f->name);
	if (ret >= sizeof(lpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	nmb = scandir(lpath, &dent, lun_select, lun_sort);
	if (nmb < 0) {
		ret = usbg_translate_error(errno);
		goto out;
	}

	for (i = nmb - 1; i > 0; --i) {
		ret = usbg_rm_dir(lpath, dent[i]->d_name);
		free(dent[i]);
		if (ret)
			goto err_free_dent_loop;
	}
	free(dent[0]);
	free(dent);

	return USBG_SUCCESS;

err_free_dent_loop:
	while (--i >= 0)
		free(dent[i]);
	free(dent[i]);
out:
	return ret;
}

struct usbg_function_type usbg_f_type_ms = {
	.name = "mass_storage",
	.alloc_inst = ms_alloc_inst,
	.free_inst = ms_free_inst,
	.set_attrs = ms_set_attrs,
	.get_attrs = ms_get_attrs,
	.cleanup_attrs = ms_cleanup_attrs,

#ifdef HAS_LIBCONFIG
	.import = ms_libconfig_import,
	.export = ms_libconfig_export,
#endif

	.remove = ms_remove,
};

