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

#define MAX_FRAMES 16
#define MAX_FORMATS 2

const char * format_names[MAX_FORMATS] = {
	UVC_PATH_STREAMING_MJPEG,
	UVC_PATH_STREAMING_UNCOMPRESSED,
};

struct formats {
	bool frames[MAX_FRAMES];
	bool frames_initiated;
};

struct usbg_f_uvc
{
	struct usbg_function func;
	struct formats formats[MAX_FORMATS];
	bool formats_initiated;
};

#define UVC_DEC_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_uvc_config_attrs, _name),     \
		.get = usbg_get_dec,				        \
		.set = usbg_set_dec,				        \
		.import = usbg_get_config_node_int,	                \
		.export = usbg_set_config_node_int,		        \
	}

#define UVC_STRING_ATTR(_name)					\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_uvc_config_attrs, _name),     \
		.get = usbg_get_string,				        \
		.set = usbg_set_string,				        \
		.export = usbg_set_config_node_string,		        \
		.import = usbg_get_config_node_string,	                \
	}

struct {
	const char *name;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} uvc_config_attr[USBG_F_UVC_CONFIG_ATTR_MAX] = {
	[USBG_F_UVC_CONFIG_MAXBURST] = UVC_DEC_ATTR(streaming_maxburst),
	[USBG_F_UVC_CONFIG_MAXPACKET] = UVC_DEC_ATTR(streaming_maxpacket),
	[USBG_F_UVC_CONFIG_INTERVAL] = UVC_DEC_ATTR(streaming_interval),
	[USBG_F_UVC_CONFIG_FUNCTION_NAME] = UVC_STRING_ATTR(function_name),
};

#undef UVC_DEC_ATTR
#undef UVC_STRING_ATTR

#define UVC_DEC_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_uvc_frame_attrs, _name),     \
		.get = usbg_get_dec,				        \
		.set = usbg_set_dec,				        \
		.import = usbg_get_config_node_int,	                \
		.export = usbg_set_config_node_int,		        \
	}

struct {
	const char *name;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} uvc_frame_attr[USBG_F_UVC_FRAME_ATTR_MAX] = {
	[USBG_F_UVC_FRAME_INDEX] = UVC_DEC_ATTR(bFrameIndex),
	[USBG_F_UVC_FRAME_CAPABILITIES] = UVC_DEC_ATTR(bmCapabilities),
	[USBG_F_UVC_FRAME_MIN_BITRATE] = UVC_DEC_ATTR(dwMinBitRate),
	[USBG_F_UVC_FRAME_MAX_BITRATE] = UVC_DEC_ATTR(dwMaxBitRate),
	[USBG_F_UVC_FRAME_MAX_VIDEO_BUFFERSIZE] = UVC_DEC_ATTR(dwMaxVideoFrameBufferSize),
	[USBG_F_UVC_FRAME_DEFAULT_INTERVAL] = UVC_DEC_ATTR(dwDefaultFrameInterval),
	[USBG_F_UVC_FRAME_INTERVAL] = UVC_DEC_ATTR(dwFrameInterval),
	[USBG_F_UVC_FRAME_HEIGHT] = UVC_DEC_ATTR(wHeight),
	[USBG_F_UVC_FRAME_WIDTH] = UVC_DEC_ATTR(wWidth),
};

#undef UVC_DEC_ATTR

#define UVC_DEC_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_uvc_format_attrs, _name),     \
		.get = usbg_get_dec,				        \
		.set = usbg_set_dec,				        \
		.import = usbg_get_config_node_int,	                \
		.export = usbg_set_config_node_int,		        \
	}

#define UVC_DEC_ATTR_RO(_name)						\
	{								\
		.name = #_name,						\
		.ro = true,						\
		.offset = offsetof(struct usbg_f_uvc_format_attrs, _name),     \
		.get = usbg_get_dec,				        \
		.export = usbg_set_config_node_int,		        \
	}

static inline int usbg_get_guid(const char *path, const char *name,
			      const char *attr, void *val)
{
	return usbg_read_buf_alloc(path, name, attr, (char **)val, GUID_BIN_LENGTH);
}

static inline int usbg_set_guid(const char *path, const char *name,
			      const char *attr, const void *val)
{
	return usbg_write_guid(path, name, attr, *(char **)val);
}

#define UVC_GUID_ATTR(_name)					\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_uvc_format_attrs, _name),     \
		.get = usbg_get_guid,					\
		.set = usbg_set_guid,					\
		.export = usbg_set_config_node_guid,			\
		.import = usbg_get_config_node_string,			\
	}

struct {
	const char *name;
	size_t offset;
	bool ro;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} uvc_format_attr[USBG_F_UVC_FORMAT_ATTR_MAX] = {
	[USBG_F_UVC_FORMAT_CONTROLS] = UVC_DEC_ATTR_RO(bmaControls),
	[USBG_F_UVC_FORMAT_INTERLACE_FLAGS] = UVC_DEC_ATTR_RO(bmInterlaceFlags),
	[USBG_F_UVC_FORMAT_ASPECTRATIO_X] = UVC_DEC_ATTR_RO(bAspectRatioX),
	[USBG_F_UVC_FORMAT_ASPECTRATIO_Y] = UVC_DEC_ATTR_RO(bAspectRatioY),
	[USBG_F_UVC_FORMAT_DEFAULT_FRAME_INDEX] = UVC_DEC_ATTR(bDefaultFrameIndex),
	[USBG_F_UVC_FORMAT_GUID_FORMAT] = UVC_GUID_ATTR(guidFormat),
	[USBG_F_UVC_FORMAT_BITS_PER_PIXEL] = UVC_DEC_ATTR(bBitsPerPixel),
	[USBG_F_UVC_FORMAT_FORMAT_INDEX] = UVC_DEC_ATTR_RO(bFormatIndex),
};


#undef UVC_DEC_ATTR
#undef UVC_DEC_ATTR_RO

static inline int frame_select(const struct dirent *dent)
{
	int ret;
	int id;

	ret = file_select(dent);
	if (!ret)
		goto out;

	ret = sscanf(dent->d_name, "frame.%d", &id);
out:
	return ret;
}

static inline int frame_sort(const struct dirent **d1, const struct dirent **d2)
{
	int ret;
	int id1, id2;

	ret = sscanf((*d1)->d_name, "frame.%d", &id1);
	if (ret != 1)
		goto err;

	ret = sscanf((*d2)->d_name, "frame.%d", &id2);
	if (ret != 1)
		goto err;

	return id1 < id2 ? -1 : id1 > id2;
err:
	/*
	 * This should not happened because dentries has been
	 * already checked by frame_select function. This
	 * error procedure is just in case.
	 */
	return -1;
}

int init_frames(struct usbg_f_uvc *uvc, int j)
{
	struct dirent **dent;
	char fpath[USBG_MAX_PATH_LENGTH];
	int nmb, i, id, ret = 0;

	nmb = snprintf(fpath, sizeof(fpath), "%s/%s/" UVC_PATH_STREAMING "/%s/",
		       uvc->func.path, uvc->func.name, format_names[j]);
	if (nmb >= sizeof(fpath)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		return ret;
	}

	nmb = scandir(fpath, &dent, frame_select, frame_sort);
	if (nmb < 0) {
		ret = usbg_translate_error(errno);
		return ret;
	}

	for (i = 0; i < nmb; ++i) {
		/* don't check the error as we know that name is valid */
		sscanf(dent[i]->d_name, "frame.%d", &id);
		uvc->formats[j].frames[id] = true;
		free(dent[i]);
	}
	free(dent);

	uvc->formats[j].frames_initiated = true;

	return 0;
}

static inline struct formats *get_formats_mask(struct usbg_f_uvc *uvc)
{
	int i, ret;

	if (!uvc->formats_initiated) {
		uvc->formats_initiated = true;
		for (i = 0; i < MAX_FORMATS; ++i) {
			ret = init_frames(uvc, i);
			if (ret)
				uvc->formats_initiated = false;
		}
	}

	return uvc->formats;
}

GENERIC_ALLOC_INST(uvc_internal, struct usbg_f_uvc, func)

static int uvc_alloc_inst(struct usbg_function_type *type,
			 usbg_function_type type_code,
			 const char *instance, const char *path,
			 struct usbg_gadget *parent,
			 struct usbg_function **f)
{
	struct usbg_f_uvc *uvcf;
	int ret;

	ret = uvc_internal_alloc_inst(type, type_code, instance,
				     path, parent, f);
	if (ret)
		goto out;

	uvcf = usbg_to_uvc_function(*f);
	if (!uvcf)
		return -ENOMEM;

	memset(uvcf->formats, 0, sizeof(uvcf->formats));
	uvcf->formats_initiated = false;
out:
	return ret;
}

GENERIC_FREE_INST(uvc, struct usbg_f_uvc, func)

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

int usbg_f_uvc_get_config_attr_val(usbg_f_uvc *uvcf, enum usbg_f_uvc_config_attr iattr,
			       union usbg_f_uvc_config_attr_val *val)
{
	char ipath[USBG_MAX_PATH_LENGTH];
	int nmb;

	nmb = snprintf(ipath, sizeof(ipath), "%s/%s/",
		       uvcf->func.path, uvcf->func.name);
	if (nmb >= sizeof(ipath))
		return USBG_ERROR_PATH_TOO_LONG;


	return uvc_config_attr[iattr].get(ipath, "",
				    uvc_config_attr[iattr].name, val);
}

int usbg_f_uvc_set_config_attr_val(usbg_f_uvc *uvcf, enum usbg_f_uvc_config_attr iattr,
			       const union usbg_f_uvc_config_attr_val *val)
{
	char ipath[USBG_MAX_PATH_LENGTH];
	int nmb;

	nmb = snprintf(ipath, sizeof(ipath), "%s/%s/",
		       uvcf->func.path, uvcf->func.name);
	if (nmb >= sizeof(ipath))
		return USBG_ERROR_PATH_TOO_LONG;

	return uvc_config_attr[iattr].set(ipath, "",
				       uvc_config_attr[iattr].name, val);
}

int usbg_f_uvc_get_config_attrs(usbg_f_uvc *uvcf, struct usbg_f_uvc_config_attrs *iattrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_UVC_CONFIG_ATTR_MIN; i < USBG_F_UVC_CONFIG_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_get_config_attr_val(uvcf, i,
					       (union usbg_f_uvc_config_attr_val *)
					       ((char *)iattrs
						+ uvc_config_attr[i].offset));
		if (ret)
			break;
	}

	return ret;
}

int usbg_f_uvc_set_config_attrs(usbg_f_uvc *uvcf, const struct usbg_f_uvc_config_attrs *iattrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_UVC_FRAME_ATTR_MIN; i < USBG_F_UVC_FRAME_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_set_config_attr_val(uvcf, i,
					       (const union usbg_f_uvc_config_attr_val *)
					       ((const char *)iattrs
						+ uvc_config_attr[i].offset));
		if (ret)
			break;
	}

	return ret;
}

int usbg_f_uvc_get_frame_attr_val(usbg_f_uvc *uvcf, const char* format, int frame_id,
			       enum usbg_f_uvc_frame_attr fattr,
			       union usbg_f_uvc_frame_attr_val *val)
{
	char fpath[USBG_MAX_PATH_LENGTH];
	int nmb;

	nmb = snprintf(fpath, sizeof(fpath), "%s/%s/" UVC_PATH_STREAMING "/%s/frame.%d/",
		       uvcf->func.path, uvcf->func.name, format, frame_id);
	if (nmb >= sizeof(fpath))
		return USBG_ERROR_PATH_TOO_LONG;

	return uvc_frame_attr[fattr].get(fpath, "",
				    uvc_frame_attr[fattr].name, val);
}

int usbg_f_uvc_set_frame_attr_val(usbg_f_uvc *uvcf, const char* format, int frame_id,
			       enum usbg_f_uvc_frame_attr fattr,
			       union usbg_f_uvc_frame_attr_val val)
{
	char fpath[USBG_MAX_PATH_LENGTH];
	int nmb;

	nmb = snprintf(fpath, sizeof(fpath), "%s/%s/" UVC_PATH_STREAMING "/%s/frame.%d/",
		       uvcf->func.path, uvcf->func.name, format, frame_id);
	if (nmb >= sizeof(fpath))
		return USBG_ERROR_PATH_TOO_LONG;

	return uvc_frame_attr[fattr].set(fpath, "",
				       uvc_frame_attr[fattr].name, &val);
}

int usbg_f_uvc_get_frame_attrs(usbg_f_uvc *uvcf, const char* format, int frame_id,
			struct usbg_f_uvc_frame_attrs *fattrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_UVC_FRAME_ATTR_MIN; i < USBG_F_UVC_FRAME_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_get_frame_attr_val(uvcf, format, frame_id, i,
					       (union usbg_f_uvc_frame_attr_val *)
					       ((char *)fattrs
						+ uvc_frame_attr[i].offset));
		if (ret)
			break;
	}

	fattrs->bFrameIndex = frame_id;

	return ret;
}

int usbg_f_uvc_set_frame_attrs(usbg_f_uvc *uvcf, const char* format, int frame_id,
			    const struct usbg_f_uvc_frame_attrs *fattrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_UVC_FRAME_ATTR_MIN; i < USBG_F_UVC_FRAME_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_set_frame_attr_val(uvcf, format, frame_id, i,
					       *(union usbg_f_uvc_frame_attr_val *)
					       ((char *)fattrs
						+ uvc_frame_attr[i].offset));
		if (ret)
			break;
	}

	return ret;
}

int usbg_f_uvc_get_format_attr_val(usbg_f_uvc *uvcf, const char* format,
			       enum usbg_f_uvc_format_attr fattr,
			       union usbg_f_uvc_format_attr_val *val)
{
	char fpath[USBG_MAX_PATH_LENGTH];
	int nmb;

	nmb = snprintf(fpath, sizeof(fpath), "%s/%s/" UVC_PATH_STREAMING "/%s/",
		       uvcf->func.path, uvcf->func.name, format);
	if (nmb >= sizeof(fpath))
		return USBG_ERROR_PATH_TOO_LONG;

	return uvc_format_attr[fattr].get(fpath, "",
				    uvc_format_attr[fattr].name, val);
}

int usbg_f_uvc_set_format_attr_val(usbg_f_uvc *uvcf, const char* format,
			       enum usbg_f_uvc_format_attr fattr,
			       union usbg_f_uvc_format_attr_val val)
{
	char fpath[USBG_MAX_PATH_LENGTH];
	int nmb;

	nmb = snprintf(fpath, sizeof(fpath), "%s/%s/" UVC_PATH_STREAMING "/%s/",
		       uvcf->func.path, uvcf->func.name, format);
	if (nmb >= sizeof(fpath))
		return USBG_ERROR_PATH_TOO_LONG;

	return uvc_format_attr[fattr].set(fpath, "",
				       uvc_format_attr[fattr].name, &val);
}

int usbg_f_uvc_get_format_attrs(usbg_f_uvc *uvcf, const char* format,
			struct usbg_f_uvc_format_attrs *fattrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_UVC_FORMAT_ATTR_MIN; i < USBG_F_UVC_FORMAT_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_get_format_attr_val(uvcf, format, i,
					       (union usbg_f_uvc_format_attr_val *)
					       ((char *)fattrs
						+ uvc_format_attr[i].offset));
		if (ret)
			break;
	}

	return ret;
}

int usbg_f_uvc_set_format_attrs(usbg_f_uvc *uvcf, const char* format,
			    const struct usbg_f_uvc_format_attrs *fattrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_UVC_FORMAT_ATTR_MIN; i < USBG_F_UVC_FORMAT_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_set_format_attr_val(uvcf, format, i,
					       *(union usbg_f_uvc_format_attr_val *)
					       ((char *)fattrs
						+ uvc_format_attr[i].offset));
		if (ret)
			break;
	}

	return ret;
}

static int uvc_create_dir(const char *path)
{
	char tmp[USBG_MAX_PATH_LENGTH];
	int nmb, ret = USBG_SUCCESS;
	char *p = NULL;
	size_t len;

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
	int ret = USBG_SUCCESS;
	int nmb;

	nmb = snprintf(oldname, sizeof(oldname), "%s/%s", path, to);
	if (nmb >= sizeof(oldname))
		return USBG_ERROR_PATH_TOO_LONG;

	nmb = snprintf(newname, sizeof(newname), "%s/%s", path, from);
	if (nmb >= sizeof(newname))
		return USBG_ERROR_PATH_TOO_LONG;

	if(symlink(oldname, newname))
		return usbg_translate_error(errno);

	return ret;
}

static int uvc_set_class(usbg_f_uvc *uvcf, char *cs)
{
	char path[USBG_MAX_PATH_LENGTH];
	char header_path[USBG_MAX_PATH_LENGTH];
	int nmb;
	int ret;

	nmb = snprintf(path, sizeof(path), "%s/%s/%s", uvcf->func.path, uvcf->func.name, cs);
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

#ifdef HAS_GADGET_SCHEMES

int usbg_f_uvc_get_nframes(const bool *frames)
{
	int nframes = 0;
	int i;

	if (!frames)
		return USBG_ERROR_INVALID_PARAM;

	for (i = 0; i < MAX_FRAMES; ++i)
		nframes += frames[i] ? 1 : 0;

	return nframes;
}

int usbg_f_uvc_create_frame(usbg_f_uvc *uvcf, const char* format, bool *frames,
			 int frame_id, struct usbg_f_uvc_frame_attrs *fattrs)
{
	char frame_path[USBG_MAX_PATH_LENGTH];
	int nmb;
	int ret;

	if (frame_id >= MAX_FRAMES)
		return USBG_ERROR_INVALID_PARAM;

	if (frames[frame_id])
		return USBG_ERROR_EXIST;

	nmb = snprintf(frame_path, sizeof(frame_path), "%s/%s/" UVC_PATH_STREAMING "/%s/frame.%d/",
		       uvcf->func.path, uvcf->func.name, format, frame_id);
	if (nmb >= sizeof(frame_path))
		return USBG_ERROR_PATH_TOO_LONG;

	ret = uvc_create_dir(frame_path);
	if (ret)
		return usbg_translate_error(errno);

	if (fattrs) {
		ret = usbg_f_uvc_set_frame_attrs(uvcf, format, frame_id, fattrs);
		if (ret)
			goto remove_frame;
	}

	frames[frame_id] = true;
	return 0;

remove_frame:
	rmdir(frame_path);
	return ret;
}

static int uvc_import_frame_attrs(struct usbg_f_uvc *uvcf, const char* format,
				  int frame_id, config_setting_t *root)
{
	union usbg_f_uvc_frame_attr_val val;
	int ret = 0;
	int i;

	for (i = USBG_F_UVC_FRAME_ATTR_MIN; i < USBG_F_UVC_FRAME_ATTR_MAX; ++i) {
		ret = uvc_frame_attr[i].import(root, uvc_frame_attr[i].name, &val);
		/* node not  found */
		if (ret == 0)
			continue;
		/* error */
		if (ret < 0)
			break;

		ret = usbg_f_uvc_set_frame_attr_val(uvcf, format, frame_id, i, val);
		if (ret)
			break;
	}

	return ret;
}

static int uvc_import_format_attrs(struct usbg_f_uvc *uvcf, const char* format,
				    config_setting_t *root)
{
	union usbg_f_uvc_format_attr_val val;
	int ret = 0;
	int i;

	for (i = USBG_F_UVC_FORMAT_ATTR_MIN; i < USBG_F_UVC_FORMAT_ATTR_MAX; ++i) {
		if (uvc_format_attr[i].ro)
			continue;

		ret = uvc_format_attr[i].import(root, uvc_format_attr[i].name, &val);
		/* node not  found */
		if (ret == 0)
			continue;
		/* error */
		if (ret < 0)
			break;

		ret = usbg_f_uvc_set_format_attr_val(uvcf, format, i, val);
		if (ret)
			break;
	}

	return ret;
}

static int uvc_import_format(struct usbg_f_uvc *uvcf, const char *format, bool *frames, config_setting_t *root)
{
	config_setting_t *frames_node, *node;
	int i, nframes;
	int ret = 0;

	frames_node = config_setting_get_member(root, "frames");
	if (!frames_node) {
		ret = USBG_ERROR_INVALID_PARAM;
		goto out;
	}

	if (!config_setting_is_list(frames_node)) {
		ret = USBG_ERROR_INVALID_TYPE;
		goto out;
	}

	nframes = config_setting_length(frames_node);
	for (i = 0; i < nframes; ++i) {
		node = config_setting_get_elem(frames_node, i);
		if (!node) {
			ret = USBG_ERROR_INVALID_FORMAT;
			goto out;
		}

		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto out;
		}
		if (!frames[i]) {
			ret = usbg_f_uvc_create_frame(uvcf, format, frames, i, NULL);
			if (ret)
				goto out;
		}
		ret = uvc_import_frame_attrs(uvcf, format, i, node);
		if (ret)
			goto out;
	}

out:
	return ret;
}

static int uvc_import_config(struct usbg_f_uvc *uvcf, config_setting_t *root)
{
	union usbg_f_uvc_config_attr_val val;
	config_setting_t *config_node;
	int ret = 0;
	int i;

	config_node = config_setting_get_member(root, "config");
	if (!config_node) {
		ret = USBG_ERROR_INVALID_PARAM;
		return ret;
	}

	if (!config_setting_is_group(config_node)) {
		ret = USBG_ERROR_INVALID_TYPE;
		return ret;
	}

	for (i = USBG_F_UVC_CONFIG_ATTR_MIN; i < USBG_F_UVC_CONFIG_ATTR_MAX; ++i) {
		ret = uvc_config_attr[i].import(config_node, uvc_config_attr[i].name, &val);
		/* node not  found */
		if (ret == 0)
			continue;
		/* error */
		if (ret < 0)
			break;

		ret = usbg_f_uvc_set_config_attr_val(uvcf, i, &val);
		if (ret)
			break;
	}

	return ret;
}

static int uvc_libconfig_import(struct usbg_function *f, config_setting_t *root)
{
	struct usbg_f_uvc *uvcf = usbg_to_uvc_function(f);
	config_setting_t *formats_node, *node;
	int nmb, ret, i, nformats;
	char fp[USBG_MAX_PATH_LENGTH];
	const char *format;
	struct formats *formats;

	formats_node = config_setting_get_member(root, "formats");
	if (!formats_node) {
		ret = USBG_ERROR_INVALID_PARAM;
		goto out;
	}

	if (!config_setting_is_group(formats_node)) {
		ret = USBG_ERROR_INVALID_TYPE;
		goto out;
	}

	nformats = config_setting_length(formats_node);
	formats = get_formats_mask(uvcf);
	for (i = 0; i < nformats; ++i) {
		node = config_setting_get_elem(formats_node, i);
		if (!node) {
			ret = USBG_ERROR_INVALID_FORMAT;
			goto out;
		}

		if (!config_setting_is_group(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto out;
		}

		format = config_setting_name(node);

		nmb = snprintf(fp, sizeof(fp), "%s/%c", format, format[0]);
		if (nmb >= sizeof(fp))
			return USBG_ERROR_PATH_TOO_LONG;

		ret = uvc_import_format(uvcf, fp, formats[i].frames, node);
		if (ret)
			goto out;

		ret = uvc_import_format_attrs(uvcf, fp, node);
		if (ret)
			goto out;
	}

	ret = uvc_import_config(uvcf, root);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = uvc_set_class(uvcf, UVC_PATH_CONTROL);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = uvc_set_class(uvcf, UVC_PATH_STREAMING);
	if (ret != USBG_SUCCESS)
		return ret;

out:
	return ret;

}

static int uvc_export_config(struct usbg_f_uvc *uvcf, config_setting_t *root)
{
	config_setting_t *config_node;
	union usbg_f_uvc_config_attr_val val;
	int i;
	int ret = 0;

	config_node = config_setting_add(root, "config", CONFIG_TYPE_GROUP);
	if (!config_node)
		goto out;

	for (i = USBG_F_UVC_CONFIG_ATTR_MIN; i < USBG_F_UVC_CONFIG_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_get_config_attr_val(uvcf, i, &val);
		if (ret)
			break;

		ret = uvc_config_attr[i].export(config_node, uvc_config_attr[i].name, &val);
		if (ret)
			break;
	}

out:
	return ret;
}

static int uvc_export_format_attrs(struct usbg_f_uvc *uvcf, const char *format,
				  config_setting_t *root)
{
	union usbg_f_uvc_format_attr_val val;
	int ret = 0;
	int i;

	for (i = USBG_F_UVC_FORMAT_ATTR_MIN; i < USBG_F_UVC_FORMAT_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_get_format_attr_val(uvcf, format, i, &val);
		if (ret)
			break;

		ret = uvc_format_attr[i].export(root, uvc_format_attr[i].name, &val);
		if (ret)
			break;
	}

	if (ret == USBG_ERROR_NOT_FOUND)
		ret = 0;

	return ret;
}

static int uvc_export_frame_attrs(struct usbg_f_uvc *uvcf, const char *format,
				  int frame_id, config_setting_t *root)
{
	union usbg_f_uvc_frame_attr_val val;
	int ret = 0;
	int i;

	for (i = USBG_F_UVC_FRAME_ATTR_MIN; i < USBG_F_UVC_FRAME_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_get_frame_attr_val(uvcf, format, frame_id, i, &val);
		if (ret)
			break;

		ret = uvc_frame_attr[i].export(root, uvc_frame_attr[i].name, &val);
		if (ret)
			break;
	}

	if (ret == USBG_ERROR_NOT_FOUND)
		ret = 0;

	return ret;
}

static int uvc_export_frames(struct usbg_f_uvc *uvcf, const char *format,
			     bool *frames, config_setting_t *root)
{
	config_setting_t *frames_node, *node;
	int i, nframes;
	int ret = 0;

	frames_node = config_setting_add(root, "frames", CONFIG_TYPE_LIST);
	if (!frames_node) {
		ret = USBG_ERROR_INVALID_FORMAT;
		goto out;
	}

	nframes = usbg_f_uvc_get_nframes(frames);
	if (nframes < 0)
		return nframes;
	for (i = 0; i < nframes; ++i) {
		node = config_setting_add(frames_node, "", CONFIG_TYPE_GROUP);
		if (!node)
			goto out;

		ret = uvc_export_frame_attrs(uvcf, format, i, node);
		if (ret)
			goto out;
	}

out:
	return ret;
}

static int uvc_libconfig_export(struct usbg_function *f, config_setting_t *root)
{
	struct usbg_f_uvc *uvcf = usbg_to_uvc_function(f);
	config_setting_t *formats_node, *node;
	char fp[USBG_MAX_PATH_LENGTH];
	struct formats *formats;
	int i, ret = 0;
	char *p;

	formats_node = config_setting_add(root, "formats", CONFIG_TYPE_GROUP);
	if (!formats_node) {
		ret = USBG_ERROR_INVALID_PARAM;
		goto out;
	}

	formats = get_formats_mask(uvcf);
	for (i = 0; i < MAX_FORMATS; ++i) {

		/* remove trailing /m and /u from format */
		strcpy(fp, format_names[i]);
		p = fp + strlen(fp) - 2; *p = '\0';

		node = config_setting_add(formats_node, fp, CONFIG_TYPE_GROUP);
		if (!node)
			goto out;

		ret = uvc_export_frames(uvcf, format_names[i], formats[i].frames, node);
		if (ret)
			goto out;

		ret = uvc_export_format_attrs(uvcf, format_names[i], node);
		if (ret)
			goto out;
	}

	ret = uvc_export_config(uvcf, root);

out:
	return ret;
}

#endif

static int uvc_set_format(char *format_path, const char *format, const struct usbg_f_uvc_format_attrs *attrs)
{
	return usbg_write_dec(format_path, format, "bDefaultFrameIndex", attrs->bDefaultFrameIndex);
}

static int uvc_set_frame(char *format_path, const char *format, const struct usbg_f_uvc_frame_attrs *attrs)
{
	char frame_path[USBG_MAX_PATH_LENGTH];
	char full_frame_path[USBG_MAX_PATH_LENGTH];
	char frame_name[32];
	int nmb, ret;

	nmb = snprintf(frame_name, sizeof(frame_name), "frame.%d", attrs->bFrameIndex);
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

	ret = usbg_write_dec(frame_path, frame_name, "dwFrameInterval", attrs->dwFrameInterval);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = usbg_write_dec(frame_path, frame_name, "dwDefaultFrameInterval", attrs->dwDefaultFrameInterval);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = usbg_write_dec(frame_path, frame_name, "dwMaxVideoFrameBufferSize", attrs->dwMaxVideoFrameBufferSize);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = usbg_write_dec(frame_path, frame_name, "dwMinBitRate", attrs->dwMinBitRate);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = usbg_write_dec(frame_path, frame_name, "dwMaxBitRate", attrs->dwMaxBitRate);
	if (ret != USBG_SUCCESS)
		return ret;

	ret = usbg_write_dec(frame_path, frame_name, "wHeight", attrs->wHeight);
	if (ret != USBG_SUCCESS)
		return ret;

	return usbg_write_dec(frame_path, frame_name, "wWidth", attrs->wWidth);
}

static int uvc_set_streaming(char *func_path, const char *format, const struct usbg_f_uvc_format_attrs *attrs)
{
	struct usbg_f_uvc_frame_attrs **frame_attrs;
	char streaming_path[USBG_MAX_PATH_LENGTH];
	int nmb, ret, i;

	nmb = snprintf(streaming_path, sizeof(streaming_path), "%s/" UVC_PATH_STREAMING, func_path);
	if (nmb >= sizeof(streaming_path))
		return USBG_ERROR_PATH_TOO_LONG;

	for(frame_attrs = attrs->frames, i = 0; frame_attrs[i]; ++i) {
		if (frame_attrs[i]) {
			ret = uvc_set_frame(streaming_path, format, frame_attrs[i]);
			if(ret != USBG_SUCCESS)
				ERROR("Error: %d", ret);
		}
	}

	ret = uvc_set_format(streaming_path, format, attrs);
	if(ret != USBG_SUCCESS)
		ERROR("Error: %d", ret);

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
	int nmb, ret = USBG_SUCCESS;

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
}

struct usbg_function_type usbg_f_type_uvc = {
	.name = "uvc",
	.alloc_inst = uvc_alloc_inst,
	.free_inst = uvc_free_inst,
	.set_attrs = uvc_set_attrs,
	.get_attrs = uvc_get_attrs,
	.cleanup_attrs = uvc_cleanup_attrs,
#ifdef HAS_GADGET_SCHEMES
	.import = uvc_libconfig_import,
	.export = uvc_libconfig_export,
#endif
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
	int nmb, i;

	if (!attrs)
		return USBG_ERROR_INVALID_PARAM;

	nmb = snprintf(path, sizeof(path), "%s/%s", uvcf->func.path, uvcf->func.name);
	if (nmb >= sizeof(path))
		return USBG_ERROR_PATH_TOO_LONG;

	for(format_attrs = attrs->formats, i = 0; format_attrs[i]; ++i) {
		ret = uvc_set_streaming(path, format_attrs[i]->format, format_attrs[i]);
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
