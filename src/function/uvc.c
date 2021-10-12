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
#include "usbg/function/uvc.h"

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <ftw.h>
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

#define UVC_PATH_CONTROL		"control"
#define UVC_PATH_HEADER			"header/h"
#define UVC_PATH_CLASS_FS		"class/fs/h"
#define UVC_PATH_CLASS_HS		"class/hs/h"
#define UVC_PATH_CLASS_SS		"class/ss/h"
#define UVC_PATH_STREAMING		"streaming"
#define UVC_PATH_STREAMING_UNCOMPRESSED	"uncompressed/u"
#define UVC_PATH_STREAMING_MJPEG	"mjpeg/m"

struct usbg_f_uvc
{
	struct usbg_function func;
};

GENERIC_ALLOC_INST(uvc, struct usbg_f_uvc, func);

GENERIC_FREE_INST(uvc, struct usbg_f_uvc, func);

static int uvc_set_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_uvc_set_attrs(usbg_to_uvc_function(f), f_attrs);
}

static int uvc_get_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_uvc_get_attrs(usbg_to_uvc_function(f), f_attrs);
}

static void uvc_cleanup_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_uvc_cleanup_attrs(f_attrs);
}

static int uvc_libconfig_import(struct usbg_function *f, config_setting_t *root)
{
	return USBG_SUCCESS;
}

static int uvc_libconfig_export(struct usbg_function *f, config_setting_t *root)
{
	return USBG_SUCCESS;
}

static int uvc_create_dir(const char *path)
{
	char tmp[USBG_MAX_PATH_LENGTH];
	char *p = NULL;
	size_t len;
	int nmb, ret = USBG_SUCCESS;

	nmb = snprintf(tmp, sizeof(tmp), "%s", path);
	if(nmb >= sizeof(tmp))
		return USBG_ERROR_PATH_TOO_LONG;

	len = strlen(tmp);
	if(tmp[len - 1] == '/')
		tmp[len - 1] = 0;

	for (p = tmp + 1; *p; p++) {
		if(*p == '/') {
			*p = 0;
			if((mkdir(tmp, S_IRWXU | S_IRWXG | S_IRWXO) != 0) && errno != EEXIST) {
				ret = usbg_translate_error(errno);
				break;
			}
			*p = '/';
		}
	}
	if(ret != USBG_SUCCESS)
		return ret;

	if((mkdir(tmp, S_IRWXU | S_IRWXG | S_IRWXO) != 0) && errno != EEXIST)
		return usbg_translate_error(errno);

	return ret;
}

static int uvc_link(char *path, char *to, char *from)
{
	char oldname[USBG_MAX_PATH_LENGTH];
	char newname[USBG_MAX_PATH_LENGTH];
	int nmb;

	nmb = snprintf(oldname, sizeof(oldname), "%s/%s", path, to);
	if (nmb >= sizeof(oldname))
		return USBG_ERROR_PATH_TOO_LONG;

	nmb = snprintf(newname, sizeof(newname), "%s/%s", path, from);
	if (nmb >= sizeof(newname))
		return USBG_ERROR_PATH_TOO_LONG;

	if(symlink(oldname, newname))
		return usbg_translate_error(errno);
}

static int uvc_set_class(char *func_path, char *cs)
{
	char path[USBG_MAX_PATH_LENGTH];
	char header_path[USBG_MAX_PATH_LENGTH];
	unsigned nmb;
	int ret;

	nmb = snprintf(path, sizeof(path), "%s/%s", func_path, cs);
	if (nmb >= sizeof(path))
		return USBG_ERROR_PATH_TOO_LONG;

	nmb = snprintf(header_path, sizeof(header_path), "%s/" UVC_PATH_HEADER, path);
	if (nmb >= sizeof(header_path))
		return USBG_ERROR_PATH_TOO_LONG;

	ret = uvc_create_dir(header_path);
	if (ret != USBG_SUCCESS)
		return ret;

	if (!strncmp(cs, UVC_PATH_STREAMING, strlen(UVC_PATH_STREAMING))) {
		char check_path[USBG_MAX_PATH_LENGTH];
		struct stat buffer;

		nmb = snprintf(check_path, sizeof(check_path), "%s/" UVC_PATH_STREAMING_UNCOMPRESSED, path);
		if (nmb >= sizeof(check_path))
			return USBG_ERROR_PATH_TOO_LONG;

		ret = stat(check_path, &buffer);
		if (!ret) {
			ret = uvc_link(path, UVC_PATH_STREAMING_UNCOMPRESSED, "header/h/u");
			if (ret != USBG_SUCCESS)
				return ret;
		}

		nmb = snprintf(check_path, sizeof(check_path), "%s/" UVC_PATH_STREAMING_MJPEG, path);
		if (nmb >= sizeof(check_path))
			return USBG_ERROR_PATH_TOO_LONG;

		ret = stat(check_path, &buffer);
		if (!ret) {
			ret = uvc_link(path, UVC_PATH_STREAMING_MJPEG, "header/h/m");
			if (ret != USBG_SUCCESS)
				return ret;
		}

		ret = uvc_link(path, UVC_PATH_HEADER, UVC_PATH_CLASS_HS);
		if (ret)
			return ret;
	}

	ret = uvc_link(path, UVC_PATH_HEADER, UVC_PATH_CLASS_FS);
	if (ret)
		return ret;

	return uvc_link(path, UVC_PATH_HEADER, UVC_PATH_CLASS_SS);
}

static int uvc_set_frame(char *format_path, char *format, const struct usbg_f_uvc_format_attrs *attrs)
{
	char frame_path[USBG_MAX_PATH_LENGTH];
	char full_frame_path[USBG_MAX_PATH_LENGTH];
	char frame_name[32];
	unsigned nmb;
	int ret;

	nmb = snprintf(frame_name, sizeof(frame_name), "%dp", attrs->height);
	if (nmb >= sizeof(frame_name))
		return USBG_ERROR_PATH_TOO_LONG;

	nmb = snprintf(frame_path, sizeof(frame_path), "%s/%s", format_path, format);
	if (nmb >= sizeof(frame_path))
		return USBG_ERROR_PATH_TOO_LONG;

	nmb = snprintf(full_frame_path, sizeof(frame_path), "%s/%s", frame_path, frame_name);
	if (nmb >= sizeof(full_frame_path))
		return USBG_ERROR_PATH_TOO_LONG;

	ret = uvc_create_dir(full_frame_path);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = usbg_write_string(frame_path, frame_name, "dwFrameInterval", attrs->dwFrameInterval);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = usbg_write_dec(frame_path, frame_name, "wHeight", attrs->height);
	if (ret != USBG_SUCCESS)
		return ret;

	return usbg_write_dec(frame_path, frame_name, "wWidth", attrs->width);
}

static int uvc_set_streaming(char *func_path, const struct usbg_f_uvc_format_attrs *attrs)
{
	char streaming_path[USBG_MAX_PATH_LENGTH];
	unsigned nmb;
	int ret;

	nmb = snprintf(streaming_path, sizeof(streaming_path), "%s/" UVC_PATH_STREAMING, func_path);
	if (nmb >= sizeof(streaming_path))
		return USBG_ERROR_PATH_TOO_LONG;

	if (attrs->format == UVC_FORMAT_UNCOMPRESSED)
		ret = uvc_set_frame(streaming_path, UVC_PATH_STREAMING_UNCOMPRESSED, attrs);
	else
		ret = uvc_set_frame(streaming_path, UVC_PATH_STREAMING_MJPEG, attrs);

	return ret;
}

static int dir_nftw_cb(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
	(void) sbuf;
	(void) type;
	(void) ftwb;
	int ret;

	ret = remove(pathname);
	if (ret < -1)
		ERROR("failed to remove %s - %s", pathname, strerror(ret));

	return 0;
}

int remove_dir(const char *dirpath)
{
	const int max_open_descs = 8;
	int ret;

	ret = nftw(dirpath, dir_nftw_cb, max_open_descs, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
	if (ret < 0) {
		ERROR("nftw failed");
		return ret;
	}

	return 0;
}

static int content_nftw_cb(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
	(void) sbuf;
	(void) type;
	(void) ftwb;
	int ret;

	if(ftwb->level == 0)
		return 0;

	ret = remove(pathname);
	if(ret < -1)
		ERROR("failed to remove %s - %s", pathname, strerror(ret));

	return 0;
}

int remove_dir_content(const char *dirpath)
{
	const int max_open_descs = 8;
	int ret;

	/* traverse in reverse order (handle directory after it's content), stay within the same file system and do not follow symbolic links */
	ret = nftw(dirpath, content_nftw_cb, max_open_descs, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
	if (ret < 0) {
		ERROR("nftw failed");
		return ret;
	}

	return 0;
}

static int uvc_remove(struct usbg_function *f, int opts)
{
	usbg_f_uvc *uvcf = usbg_to_uvc_function(f);
	char streaming_path[USBG_MAX_PATH_LENGTH];
	char control_path[USBG_MAX_PATH_LENGTH];
	char path[USBG_MAX_PATH_LENGTH];
	int ret = USBG_SUCCESS;
	unsigned nmb;

	nmb = snprintf(path, sizeof(path), "%s/%s", uvcf->func.path, uvcf->func.name);
	if (nmb >= sizeof(path))
		return USBG_ERROR_PATH_TOO_LONG;

	nmb = snprintf(streaming_path, sizeof(streaming_path), "%s/streaming", path);
	if (nmb >= sizeof(streaming_path))
		return USBG_ERROR_PATH_TOO_LONG;

	nmb = snprintf(control_path, sizeof(control_path), "%s/control", path);
	if (nmb >= sizeof(control_path))
		return USBG_ERROR_PATH_TOO_LONG;

	if(remove_dir_content(streaming_path) < 0)
		return USBG_ERROR_PATH_TOO_LONG;

	if(remove_dir_content(control_path) < 0)
		return USBG_ERROR_PATH_TOO_LONG;

	if(remove_dir(streaming_path) < 0)
		return USBG_ERROR_PATH_TOO_LONG;

	if(remove_dir(control_path) < 0)
		return USBG_ERROR_PATH_TOO_LONG;

	return ret;
};

struct usbg_function_type usbg_f_type_uvc = {
	.name = "uvc",
	.alloc_inst = uvc_alloc_inst,
	.free_inst = uvc_free_inst,
	.set_attrs = uvc_set_attrs,
	.get_attrs = uvc_get_attrs,
	.cleanup_attrs = uvc_cleanup_attrs,
	.import = uvc_libconfig_import,
	.export = uvc_libconfig_export,
	.remove = uvc_remove,
};

/* API implementation */

usbg_f_uvc *usbg_to_uvc_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_uvc ?
			container_of(f, struct usbg_f_uvc, func) : NULL;
}

usbg_function *usbg_from_uvc_function(usbg_f_uvc *ff)
{
	return &ff->func;
}

int usbg_f_uvc_get_attrs(usbg_f_uvc *uvcf, struct usbg_f_uvc_attrs *attrs)
{
	return USBG_SUCCESS;
}

int usbg_f_uvc_set_attrs(usbg_f_uvc *uvcf, const struct usbg_f_uvc_attrs *attrs)
{
	char path[USBG_MAX_PATH_LENGTH];
	struct usbg_f_uvc_format_attrs **format_attrs;
	int ret = USBG_SUCCESS;
	unsigned nmb;
	int i;

	if (!attrs)
		return USBG_ERROR_INVALID_PARAM;

	nmb = snprintf(path, sizeof(path), "%s/%s", uvcf->func.path, uvcf->func.name);
	if (nmb >= sizeof(path))
		return USBG_ERROR_PATH_TOO_LONG;

	for(format_attrs = attrs->formats, i = 0; format_attrs[i]; ++i) {
		ret = uvc_set_streaming(path, format_attrs[i]);
		if(ret != USBG_SUCCESS)
			ERROR("Error: %d", ret);
	}

	ret = uvc_set_class(uvcf, UVC_PATH_CONTROL);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = uvc_set_class(uvcf, UVC_PATH_STREAMING);
	if (ret != USBG_SUCCESS)
		return ret;

	return ret;
}
