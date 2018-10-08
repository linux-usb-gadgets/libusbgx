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

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

struct usbg_f_uvc
{
	struct usbg_function func;
};

#define UVC_DEC_ATTR(_name)                                   \
  {                                                           \
    .name = #_name,                                           \
    .offset = offsetof(struct usbg_f_uvc_attrs, _name),       \
    .get = usbg_get_dec,                                      \
    .set = usbg_set_dec,                                      \
    .import = usbg_get_config_node_int,                       \
    .export = usbg_set_config_node_int,                       \
  }

#define UVC_FORMAT_DESC_ATTR(_name)                           \
  {                                                           \
    .name = #_name,                                           \
    .offset = offsetof(struct usbg_f_uvc_attrs, _name),       \
    .get = usbg_f_uvc_get_format_descs,                       \
    .set = usbg_f_uvc_set_format_descs,                       \
    .import = NULL,                                           \
    .export = NULL,                                           \
  }

#define UVC_FRAME_DEC_ATTR(_name)                             \
  {                                                           \
    .name = #_name,                                           \
    .offset = offsetof(struct usbg_f_uvc_frame_attrs, _name), \
    .get = usbg_get_dec,                                      \
    .set = usbg_set_dec,                                      \
    .import = usbg_get_config_node_int,                       \
    .export = usbg_set_config_node_int,                       \
  }

#define UVC_FRAME_STRING_ATTR(_name)                          \
  {                                                           \
    .name = #_name,                                           \
    .offset = offsetof(struct usbg_f_uvc_frame_attrs, _name), \
    .get = usbg_get_string,                                   \
    .set = usbg_set_string,                                   \
    .export = usbg_set_config_node_string,                    \
    .import = usbg_get_config_node_string,                    \
  }

static struct
{
	const char *name;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} uvc_attr[USBG_F_UVC_ATTR_MAX] = {
	[USBG_F_UVC_STREAMING_INTERVAL] = UVC_DEC_ATTR(streaming_interval),
	[USBG_F_UVC_STREAMING_MAX_BURST] =		UVC_DEC_ATTR(streaming_maxburst),
	[USBG_F_UVC_STREAMING_MAX_PACKET] = UVC_DEC_ATTR(streaming_maxpacket),
	[USBG_F_UVC_FORMAT_DESCS] =		UVC_FORMAT_DESC_ATTR(format_descriptors),
};

static struct
{
	const char *name;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} uvc_frame_attr[USBG_F_UVC_FRAME_ATTR_MAX] = {
	[USBG_F_UVC_FRAME_ATTR_BM_CAPABILITIES] = UVC_FRAME_DEC_ATTR(bmCapabilities),
	[USBG_F_UVC_FRAME_ATTR_DW_FRAME_INTERVAL] = UVC_FRAME_STRING_ATTR(dwFrameInterval),
	[USBG_F_UVC_FRAME_ATTR_DW_DEFAULT_FRAME_INTERVAL] =	UVC_FRAME_DEC_ATTR(dwDefaultFrameInterval),
	[USBG_F_UVC_FRAME_ATTR_DW_MAX_VIDEO_FRAME_BUFFER_SIZE] = UVC_FRAME_DEC_ATTR(dwMaxVideoFrameBufferSize),
	[USBG_F_UVC_FRAME_ATTR_DW_MAX_BIT_RATE] = UVC_FRAME_DEC_ATTR(dwMaxBitRate),
	[USBG_F_UVC_FRAME_ATTR_DW_MIN_BIT_RATE] = UVC_FRAME_DEC_ATTR(dwMinBitRate),
	[USBG_F_UVC_FRAME_ATTR_W_HEIGHT] = UVC_FRAME_DEC_ATTR(wHeight),
	[USBG_F_UVC_FRAME_ATTR_W_WIDTH] = UVC_FRAME_DEC_ATTR(wWidth),
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
	usbg_f_uvc_cleanup_attrs(f_attrs);
}

static int uvc_libconfig_import(struct usbg_function *f, config_setting_t *root)
{
	return USBG_SUCCESS;
}

static int uvc_libconfig_export(struct usbg_function *f, config_setting_t *root)
{
	return USBG_SUCCESS;
}

static int uvc_get_sub_dirs(const char *dir_path, char *sub_dirs, size_t size)
{
	DIR *dir;
	struct dirent *entry;
	int ret = 0, nmb;

	if(!(dir = opendir(dir_path))) {
		ret = 0;
		goto out;
	}

	while ((entry = readdir(dir)) != NULL) {
		if(entry->d_type != DT_DIR
				|| (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))
			continue;

		nmb = snprintf(&sub_dirs[ret], (size - ret), "%s", entry->d_name);
		ret += (nmb + 1);
	}
	closedir(dir);

	out: return ret;
}

static int uvc_create_dir(const char *path)
{
	char tmp[USBG_MAX_PATH_LENGTH];
	char *p = NULL;
	char delim[] = "/";
	size_t len;
	int nmb, ret = USBG_SUCCESS;

	nmb = snprintf(tmp, sizeof(tmp), "%s", path);
	if(nmb >= sizeof(tmp)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

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
		goto out;

	if((mkdir(tmp, S_IRWXU | S_IRWXG | S_IRWXO) != 0) && errno != EEXIST)
		ret = usbg_translate_error(errno);

	out: return ret;
}

static int uvc_link_streaming_header(const char *path, const char *name)
{
	char p1[USBG_MAX_PATH_LENGTH], p2[USBG_MAX_PATH_LENGTH];
	int i, nmb, ret = USBG_SUCCESS;
	struct stat sb;

	nmb = snprintf(p2, sizeof(p2), "%s/%s" UVC_PATH_STREAMING_HEADER, path, name);
	if(nmb >= sizeof(p2)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	for (i = 0; i < 3; i++) {
		switch(i)
		{
			case 0:
				nmb = snprintf(p1, sizeof(p1), "%s/%s" UVC_PATH_STREAMING_CLASS_FS, path,
						name);
				break;
			case 1:
				nmb = snprintf(p1, sizeof(p1), "%s/%s" UVC_PATH_STREAMING_CLASS_HS, path,
						name);
				break;
			case 2:
				nmb = snprintf(p1, sizeof(p1), "%s/%s" UVC_PATH_STREAMING_CLASS_SS, path,
						name);
				break;
			default:
				break;
		}

		if(nmb >= sizeof(p1)) {
			ret = USBG_ERROR_PATH_TOO_LONG;
			break;
		}

		if(lstat(p1, &sb))
			break;

		ret = symlink(p2, p1);
		if(ret != 0) {
			ret = usbg_translate_error(errno);
			break;
		}
	}

	out: return ret;
}

static int uvc_link_desc(const char *path, const char *name, const char *format,
		const char *spec_format)
{
	char p1[USBG_MAX_PATH_LENGTH], p2[USBG_MAX_PATH_LENGTH];
	int nmb, ret = USBG_SUCCESS;
	struct stat sb;

	nmb = snprintf(p1, sizeof(p1), "%s/%s" UVC_PATH_STREAMING_HEADER "/%s", path,
			name, spec_format);
	if(nmb >= sizeof(p1)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	nmb = snprintf(p2, sizeof(p2), "%s/%s" UVC_PATH_STREAMING "/%s/%s", path, name,
			format, spec_format);
	if(nmb >= sizeof(p2)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	if(lstat(p1, &sb))
		return ret;

	ret = symlink(p2, p1);
	if(ret != USBG_SUCCESS)
		ret = usbg_translate_error(errno);

	out: return ret;
}

static int uvc_link_specific_format_descs(const char *path, const char *name,
		struct usbg_f_uvc_format_descs *format_descriptors)
{
	char p1[USBG_MAX_PATH_LENGTH], p2[USBG_MAX_PATH_LENGTH];
	int i, nmb, ret = USBG_SUCCESS;

	if(ret != USBG_SUCCESS)
		goto out;

	nmb = snprintf(p1, sizeof(p1), "%s/%s" UVC_PATH_STREAMING_HEADER, path, name);
	if(nmb >= sizeof(p1)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = uvc_create_dir(p1);
	if(ret != USBG_SUCCESS)
		goto out;

	if(format_descriptors->uncompressed != NULL) {
		for (i = 0; i < MAX_FORMAT_DESCRIPTORS; i++) {
			if(format_descriptors->uncompressed[i].specific_format_descriptor[0] == '\0')
				break;

			ret = uvc_link_desc(path, name, "uncompressed",
					format_descriptors->uncompressed[i].specific_format_descriptor);
			if(ret != USBG_SUCCESS)
				goto out;
		}
	}

	if(format_descriptors->mjpeg != NULL) {
		for (i = 0; i < MAX_FORMAT_DESCRIPTORS; i++) {
			if(format_descriptors->mjpeg[i].specific_format_descriptor[0] == '\0')
				break;

			ret = uvc_link_desc(path, name, "mjpeg",
					format_descriptors->mjpeg[i].specific_format_descriptor);
			if(ret != USBG_SUCCESS)
				goto out;
		}
	}

	ret = uvc_link_streaming_header(path, name);

	out: return ret;
}

static int uvc_set_control_header(const char *path, const char *name)
{
	char p1[USBG_MAX_PATH_LENGTH], p2[USBG_MAX_PATH_LENGTH];
	int nmb, ret = USBG_SUCCESS;
	DIR *dir;

	nmb = snprintf(p1, sizeof(p1), "%s/%s" UVC_PATH_CONTROL_HEADER, path, name);
	if(nmb >= sizeof(p1)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	dir = opendir(p1);
	if(!dir) {
		ret = uvc_create_dir(p1);
		if(ret != USBG_SUCCESS) {
			ret = usbg_translate_error(errno);
			goto out;
		}

		nmb = snprintf(p2, sizeof(p2), "%s/%s" UVC_PATH_CONTROL_CLASS_FS, path, name);
		if(nmb >= sizeof(p2)) {
			ret = USBG_ERROR_PATH_TOO_LONG;
			goto out;
		}
		ret = symlink(p1, p2);
		if(ret != USBG_SUCCESS) {
			ret = usbg_translate_error(errno);
			goto out;
		}

		nmb = snprintf(p2, sizeof(p2), "%s/%s" UVC_PATH_CONTROL_CLASS_SS, path, name);
		if(nmb >= sizeof(p2)) {
			ret = USBG_ERROR_PATH_TOO_LONG;
			goto out;
		}

		ret = symlink(p1, p2);
		if(ret != USBG_SUCCESS)
			ret = usbg_translate_error(errno);
	}
	else {
		closedir(dir);
	}

	out: return ret;
}

static int uvc_get_frame_attr_val(const char *path, const char *name,
		enum usbg_f_uvc_frame_attr attr, union usbg_f_uvc_frame_attr_val *val,
		char *sub_path)
{
	char p[USBG_MAX_PATH_LENGTH];
	int nmb;

	nmb = snprintf(p, sizeof(p), "%s%s", name, sub_path);
	if(nmb >= sizeof(p))
		return USBG_ERROR_PATH_TOO_LONG;

	return uvc_frame_attr[attr].get(path, p, uvc_frame_attr[attr].name, val);
}

static int uvc_set_frame_attr_val(const char *path, const char *name,
		enum usbg_f_uvc_frame_attr attr, union usbg_f_uvc_frame_attr_val val,
		char *sub_path)
{
	char p[USBG_MAX_PATH_LENGTH];
	int nmb;

	nmb = snprintf(p, sizeof(p), "%s%s", name, sub_path);
	if(nmb >= sizeof(p))
		return USBG_ERROR_PATH_TOO_LONG;

	return uvc_frame_attr[attr].set(path, p, uvc_frame_attr[attr].name, &val);
}

static int uvc_get_frame_descs(const char *path, const char *name,
		const char *path_format_desc,
		struct usbg_f_uvc_frame_descriptor *frame_descriptors)
{
	char p[USBG_MAX_PATH_LENGTH];
	char specific_frame_descriptor[USBG_MAX_PATH_LENGTH];
	int frame_desc_size, position = 0, ret = USBG_SUCCESS, i, nmb;
	struct usbg_f_uvc_frame_descriptor *frame_descriptor = &frame_descriptors[0];

	nmb = snprintf(p, sizeof(p), "%s/%s%s", path, name, path_format_desc);
	if(nmb >= sizeof(p)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	frame_desc_size = uvc_get_sub_dirs(p, specific_frame_descriptor,
			sizeof(specific_frame_descriptor));

	if(frame_desc_size != 0) {
		while (position < frame_desc_size
				&& (frame_descriptor <= &frame_descriptors[MAX_FRAME_DESCRIPTORS - 1])) {
			frame_descriptor->specific_frame_descriptor = strdup(
					&specific_frame_descriptor[position]);
			if(frame_descriptor->specific_frame_descriptor == NULL) {
				ret = USBG_ERROR_NO_MEM;
				break;
			}

			nmb = snprintf(p, sizeof(p), "%s/%s", path_format_desc,
					frame_descriptor->specific_frame_descriptor);
			if(nmb >= sizeof(p)) {
				ret = USBG_ERROR_PATH_TOO_LONG;
				break;
			}

			for (i = USBG_F_UVC_FRAME_ATTR_MIN; i < USBG_F_UVC_FRAME_ATTR_MAX; ++i) {
				ret =
						uvc_get_frame_attr_val(path, name, i,
								(union usbg_f_uvc_frame_attr_val *) ((char *) &(frame_descriptor->frame_attrs)
										+ uvc_frame_attr[i].offset), p);
				if(ret != USBG_SUCCESS)
					break;
			}

			position += strlen(frame_descriptor->specific_frame_descriptor) + 1;
			frame_descriptor++;
		}

		if(position < frame_desc_size)
			ret = USBG_ERROR_NO_MEM;
	}

	out: return ret;
}

int uvc_set_frame_desc(const char *path, const char *name,
		const char *format_desc, const char *spec_format_desc,
		const struct usbg_f_uvc_frame_descriptor *frame_descriptor)
{
	char p[USBG_MAX_PATH_LENGTH];
	int ret = USBG_SUCCESS;
	int nmb, i;

	nmb = snprintf(p, sizeof(p), "%s/%s" UVC_PATH_STREAMING "/%s/%s/%s", path,
			name, format_desc, spec_format_desc,
			frame_descriptor->specific_frame_descriptor);
	if(nmb >= sizeof(p)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = uvc_create_dir(p);
	if(ret != USBG_SUCCESS)
		goto out;

	nmb = snprintf(p, sizeof(p), UVC_PATH_STREAMING "/%s/%s/%s", format_desc,
			spec_format_desc, frame_descriptor->specific_frame_descriptor);
	if(nmb >= sizeof(p)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	for (i = USBG_F_UVC_FRAME_ATTR_MIN; i < USBG_F_UVC_FRAME_ATTR_MAX; ++i) {
		ret =
				uvc_set_frame_attr_val(path, name, i,
						*(union usbg_f_uvc_frame_attr_val *) ((char *) &(frame_descriptor->frame_attrs)
								+ uvc_frame_attr[i].offset), p);
		if(ret != USBG_SUCCESS)
			break;
	}

	out: return ret;
}

static int uvc_get_uncompressed_format_descs(const char *path, const char *name,
		struct usbg_f_uvc_format_desc_uncompressed *format_descs_uncompressed)
{
	struct usbg_f_uvc_format_desc_uncompressed *uncompressed =
			format_descs_uncompressed;
	char p[USBG_MAX_PATH_LENGTH];
	char specific_format_descriptors[USBG_MAX_PATH_LENGTH];
	int nmb, format_descs_size, position = 0, ret = USBG_SUCCESS;

	nmb = snprintf(p, sizeof(p), "%s/%s" UVC_PATH_STREAMING UVC_PATH_UNCOMPRESSED,
			path, name);
	if(nmb >= sizeof(p)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	format_descs_size = uvc_get_sub_dirs(p, specific_format_descriptors,
			sizeof(specific_format_descriptors));

	if(format_descs_size != 0) {
		while (position < format_descs_size
				&& (uncompressed <= &format_descs_uncompressed[MAX_FORMAT_DESCRIPTORS - 1])) {
			uncompressed->specific_format_descriptor = strdup(
					&specific_format_descriptors[position]);
			if(uncompressed->specific_format_descriptor == NULL) {
				ret = USBG_ERROR_NO_MEM;
				break;
			}

			nmb = snprintf(p, sizeof(p), UVC_PATH_STREAMING UVC_PATH_UNCOMPRESSED "/%s",
					uncompressed->specific_format_descriptor);
			if(nmb >= sizeof(p)) {
				ret = USBG_ERROR_PATH_TOO_LONG;
				break;
			}

			ret = uvc_get_frame_descs(path, name, p, uncompressed->frame_descriptor);
			if(ret != USBG_SUCCESS)
				goto out;

			position += strlen(&specific_format_descriptors[position]) + 1;
			uncompressed++;
		}

		if(position < format_descs_size)
			ret = USBG_ERROR_NO_MEM;
	}

	out: return ret;
}

static int uvc_set_uncompressed_format_descs(const char *path, const char *name,
		const char *attr, struct usbg_f_uvc_format_desc_uncompressed *format_desc)
{
	int m, n, ret = USBG_SUCCESS;

	for (m = 0; m < MAX_FORMAT_DESCRIPTORS; m++) {
		if(format_desc[m].specific_format_descriptor[0] == '\0')
			break;

		for (n = 0; n < MAX_FRAME_DESCRIPTORS; n++) {
			if(format_desc[m].frame_descriptor[n].specific_frame_descriptor[0] == '\0')
				break;

			ret = uvc_set_frame_desc(path, name, attr,
					format_desc[m].specific_format_descriptor,
					&(format_desc[m].frame_descriptor[n]));
			if(ret != USBG_SUCCESS)
				break;
		}
	}
	return ret;
}

static int uvc_get_mjpeg_format_descs(const char *path, const char *name,
		struct usbg_f_uvc_format_desc_mjpeg *format_descs_mjpeg)
{
	struct usbg_f_uvc_format_desc_mjpeg *mjpeg = format_descs_mjpeg;
	char p[USBG_MAX_PATH_LENGTH];
	char specific_format_descriptors[USBG_MAX_PATH_LENGTH];
	int nmb, format_descs_size, position = 0, ret = USBG_SUCCESS;

	nmb = snprintf(p, sizeof(p), "%s/%s" UVC_PATH_STREAMING UVC_PATH_MJPEG, path,
			name);
	if(nmb >= sizeof(p)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	format_descs_size = uvc_get_sub_dirs(p, specific_format_descriptors,
			sizeof(specific_format_descriptors));

	if(format_descs_size != 0) {
		while (position < format_descs_size
				&& (mjpeg <= &format_descs_mjpeg[MAX_FORMAT_DESCRIPTORS - 1])) {
			mjpeg->specific_format_descriptor = strdup(
					&specific_format_descriptors[position]);
			if(mjpeg->specific_format_descriptor == NULL) {
				ret = USBG_ERROR_NO_MEM;
				break;
			}

			nmb = snprintf(p, sizeof(p), UVC_PATH_STREAMING UVC_PATH_MJPEG "/%s",
					mjpeg->specific_format_descriptor);
			if(nmb >= sizeof(p)) {
				ret = USBG_ERROR_PATH_TOO_LONG;
				break;
			}

			ret = uvc_get_frame_descs(path, name, p, mjpeg->frame_descriptor);
			if(ret != USBG_SUCCESS)
				goto out;

			position += strlen(&specific_format_descriptors[position]) + 1;
			mjpeg++;
		}

		if(position < format_descs_size)
			ret = USBG_ERROR_NO_MEM;
	}

	out: return ret;
}

static int uvc_set_mjpeg_format_descs(const char *path, const char *name,
		const char *attr, struct usbg_f_uvc_format_desc_mjpeg *format_desc)
{
	int m, n, ret = USBG_SUCCESS;

	for (m = 0; m < MAX_FORMAT_DESCRIPTORS; m++) {
		if(format_desc[m].specific_format_descriptor[0] == '\0')
			break;

		for (n = 0; n < MAX_FRAME_DESCRIPTORS; n++) {
			if(format_desc[m].frame_descriptor[n].specific_frame_descriptor[0] == '\0')
				break;

			ret = uvc_set_frame_desc(path, name, attr,
					format_desc[m].specific_format_descriptor,
					&(format_desc[m].frame_descriptor[n]));
			if(ret != USBG_SUCCESS)
				break;
		}
	}
	return ret;
}

int usbg_f_uvc_get_format_descs(const char *path, const char *name,
		const char *attr, void *val)
{
	struct usbg_f_uvc_format_descs *format_descs =
			(struct usbg_f_uvc_format_descs *) val;
	int ret = USBG_SUCCESS;

	ret = uvc_get_uncompressed_format_descs(path, name,
			format_descs->uncompressed);
	if(ret != USBG_SUCCESS)
		goto out;

	ret = uvc_get_mjpeg_format_descs(path, name, format_descs->mjpeg);

	out: return ret;
}

int usbg_f_uvc_set_format_descs(const char *path, const char *name,
		const char *attr, void *val)
{
	struct usbg_f_uvc_format_descs *format_descs =
			(struct usbg_f_uvc_format_descs *) val;
	int ret = USBG_SUCCESS;

	ret = uvc_set_control_header(path, name);
	if(ret != USBG_SUCCESS)
		goto out;

	if(format_descs->uncompressed != NULL) {
		ret = uvc_set_uncompressed_format_descs(path, name, UVC_PATH_UNCOMPRESSED,
				format_descs->uncompressed);
		if(ret != USBG_SUCCESS)
			goto out;
	}

	if(format_descs->mjpeg != NULL) {
		ret = uvc_set_mjpeg_format_descs(path, name, UVC_PATH_MJPEG,
				format_descs->mjpeg);
		if(ret != USBG_SUCCESS)
			goto out;
	}

	ret = uvc_link_specific_format_descs(path, name, format_descs);

	out: return ret;
}

struct usbg_function_type usbg_f_type_uvc = { .name = "uvc", .alloc_inst =
		uvc_alloc_inst, .free_inst = uvc_free_inst, .set_attrs = uvc_set_attrs,
		.get_attrs = uvc_get_attrs, .cleanup_attrs = uvc_cleanup_attrs, .import =
				uvc_libconfig_import, .export = uvc_libconfig_export, };

/* API implementation */

usbg_f_uvc *usbg_to_uvc_function(usbg_function *f)
{
	return
			f->ops == &usbg_f_type_uvc ? container_of(f, struct usbg_f_uvc, func) : NULL;
}

usbg_function *usbg_from_uvc_function(usbg_f_uvc *ff)
{
	return &ff->func;
}

int usbg_f_uvc_get_attrs(usbg_f_uvc *af, struct usbg_f_uvc_attrs *attrs)
{
	int i, ret = USBG_SUCCESS;

	for (i = USBG_F_UVC_ATTR_MIN; i < USBG_F_UVC_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_get_attr_val(af, i,
				(union usbg_f_uvc_attr_val *) ((char *) attrs + uvc_attr[i].offset));
		if(ret != USBG_SUCCESS)
			break;
	}
	return ret;
}

int usbg_f_uvc_set_attrs(usbg_f_uvc *af, const struct usbg_f_uvc_attrs *attrs)
{
	int i, ret = USBG_SUCCESS;

	for (i = USBG_F_UVC_ATTR_MIN; i < USBG_F_UVC_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_set_attr_val(af, i,
				*(union usbg_f_uvc_attr_val *) ((char *) attrs + uvc_attr[i].offset));
		if(ret != USBG_SUCCESS)
			break;
	}

	return ret;
}

int usbg_f_uvc_get_attr_val(usbg_f_uvc *af, enum usbg_f_uvc_attr attr,
		union usbg_f_uvc_attr_val *val)
{
	return uvc_attr[attr].get(af->func.path, af->func.name, uvc_attr[attr].name,
			val);
}

int usbg_f_uvc_set_attr_val(usbg_f_uvc *af, enum usbg_f_uvc_attr attr,
		union usbg_f_uvc_attr_val val)
{
	return uvc_attr[attr].set(af->func.path, af->func.name, uvc_attr[attr].name,
			&val);
}
