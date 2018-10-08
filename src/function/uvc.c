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
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

#define UVC_PATH_CONTROL		"control/"
#define UVC_PATH_HEADER			"header/"
#define UVC_PATH_CLASS_FS		"class/fs"
#define UVC_PATH_CLASS_HS		"class/hs"
#define UVC_PATH_CLASS_SS		"class/ss"
#define UVC_PATH_STREAMING		"streaming/"
#define UVC_PATH_STREAMING_UNCOMPRESSED	"uncompressed/"

struct usbg_f_uvc
{
	struct usbg_function func;
};
static int uvc_read_buf_16(const char *path, const char *name,
		      const char *attr, void *val);
static int uvc_write_buf_16(const char *path, const char *name,
			      const char *attr, void *val);

#define UVC_DEC_ATTR(_name, _struct)				\
{								\
	.name = #_name,						\
	.offset = offsetof(struct _struct, _name),		\
	.get = usbg_get_dec,					\
	.set = usbg_set_dec,					\
	.import = usbg_get_config_node_int,			\
	.export = usbg_set_config_node_int,			\
}

#define UVC_DEC_ATTR_RO(_name, _struct)				\
{								\
	.name = #_name,						\
	.offset = offsetof(struct _struct, _name),		\
	.get = usbg_get_dec,					\
	.set = NULL,					\
	.import = usbg_get_config_node_int,			\
	.export = usbg_set_config_node_int,			\
}

#define UVC_STRING_ATTR(_name, _struct)				\
{                                                           	\
	.name = #_name,                                         \
	.offset = offsetof(struct _struct, _name), 		\
	.get = usbg_get_string,					\
	.set = usbg_set_string,					\
	.export = usbg_set_config_node_string,			\
	.import = usbg_get_config_node_string,			\
}

#define UVC_BUF_16_ATTR(_name, _struct)				\
{                                                           	\
	.name = #_name,                                         \
	.offset = offsetof(struct _struct, _name), 		\
	.get = uvc_read_buf_16,					\
	.set = uvc_write_buf_16,				\
	.export = usbg_set_config_node_string,			\
	.import = usbg_get_config_node_string,			\
}

struct uvc_attr_t
{
	const char *name;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
};

static struct uvc_attr_t uvc_attr[USBG_F_UVC_ATTR_MAX] = {
	[USBG_F_UVC_STREAMING_INTERVAL] = UVC_DEC_ATTR(streaming_interval, usbg_f_uvc_attrs),
	[USBG_F_UVC_STREAMING_MAX_BURST] = UVC_DEC_ATTR(streaming_maxburst, usbg_f_uvc_attrs),
	[USBG_F_UVC_STREAMING_MAX_PACKET] = UVC_DEC_ATTR(streaming_maxpacket, usbg_f_uvc_attrs),
};

static struct uvc_attr_t uvc_frame_attr[USBG_F_UVC_FRM_ATTR_MAX] = {
	[USBG_F_UVC_FRM_ATTR_B_FRAME_INDEX] = UVC_DEC_ATTR_RO(bFrameIndex, usbg_f_uvc_frame_desc),
	[USBG_F_UVC_FRM_ATTR_DW_FRAME_INTERVAL] = UVC_STRING_ATTR(dwFrameInterval, usbg_f_uvc_frame_desc),
	[USBG_F_UVC_FRM_ATTR_DW_DEFAULT_FRAME_INTERVAL] = UVC_DEC_ATTR(dwDefaultFrameInterval, usbg_f_uvc_frame_desc),
	[USBG_F_UVC_FRM_ATTR_DW_MAX_VIDEO_FRAME_BUFFER_SIZE] = UVC_DEC_ATTR(dwMaxVideoFrameBufferSize, usbg_f_uvc_frame_desc),
	[USBG_F_UVC_FRM_ATTR_DW_MAX_BIT_RATE] = UVC_DEC_ATTR(dwMaxBitRate, usbg_f_uvc_frame_desc),
	[USBG_F_UVC_FRM_ATTR_DW_MIN_BIT_RATE] = UVC_DEC_ATTR(dwMinBitRate, usbg_f_uvc_frame_desc),
	[USBG_F_UVC_FRM_ATTR_W_HEIGHT] = UVC_DEC_ATTR(wHeight, usbg_f_uvc_frame_desc),
	[USBG_F_UVC_FRM_ATTR_W_WIDTH] = UVC_DEC_ATTR(wWidth, usbg_f_uvc_frame_desc),
	[USBG_F_UVC_FRM_ATTR_BM_CAPABILITIES] = UVC_DEC_ATTR(bmCapabilities, usbg_f_uvc_frame_desc),
};

static struct uvc_attr_t uvc_uncompressed_format_attr[USBG_F_UVC_U_FMT_ATTR_MAX] = {
	[USBG_F_UVC_U_FMT_ATTR_B_FORMAT_INDEX] = UVC_DEC_ATTR_RO(bFormatIndex, usbg_f_uvc_u_format_desc),
	[USBG_F_UVC_U_FMT_ATTR_BMA_CONTROLS] =	UVC_DEC_ATTR_RO(bmaControls, usbg_f_uvc_u_format_desc),
	[USBG_F_UVC_U_FMT_ATTR_BM_INTERFACE_FLAGS] = UVC_DEC_ATTR_RO(bmInterfaceFlags, usbg_f_uvc_u_format_desc),
	[USBG_F_UVC_U_FMT_ATTR_B_ASPECT_RATIO_Y] = UVC_DEC_ATTR_RO(bAspectRatioY, usbg_f_uvc_u_format_desc),
	[USBG_F_UVC_U_FMT_ATTR_B_ASPECT_RATIO_X] = UVC_DEC_ATTR_RO(bAspectRatioX, usbg_f_uvc_u_format_desc),
	[USBG_F_UVC_U_FMT_ATTR_B_DEFAULT_FRAME_INDEX] = UVC_DEC_ATTR(bDefaultFrameIndex, usbg_f_uvc_u_format_desc),
	[USBG_F_UVC_U_FMT_ATTR_B_BITS_PER_PIXEL] = UVC_DEC_ATTR(bBitsPerPixel, usbg_f_uvc_u_format_desc),
	[USBG_F_UVC_U_FMT_ATTR_GUID_FORMAT] = UVC_BUF_16_ATTR(guidFormat, usbg_f_uvc_u_format_desc),
};

GENERIC_ALLOC_INST(uvc, struct usbg_f_uvc, func);

GENERIC_FREE_INST(uvc, struct usbg_f_uvc, func);

static int uvc_read_buf_16(const char *path, const char *name,
		      const char *attr, void *val)
{
	char buf[16];
	char *new_buf = NULL;
	int ret;

	ret = usbg_read_buf_limited(path, name, attr, buf, sizeof(buf));
	if (ret != USBG_SUCCESS)
		goto out;

	new_buf = strdup(buf);
	if (!new_buf) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	val = (void *)new_buf;
out:
	return ret;
}

static int uvc_write_buf_16(const char *path, const char *name,
			      const char *attr, void *val)
{
	int ret;
	ret = usbg_write_buf(path, name, attr, (char *)val, 16);
	if (ret > 0)
		ret = 0;

	return ret;
}

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

static int usbg_f_uvc_link_control_class(unsigned char *control_path,
		unsigned char *class_path,
		struct usbg_f_uvc_control_header_desc *header)
{
	char class_link[USBG_MAX_PATH_LENGTH];
	int nmb, ret;

	if (!header) {
		ret = USBG_ERROR_MISSING_TAG;
		goto out;
	}

	nmb = snprintf(class_link, sizeof(class_link), "%s%s/%s", control_path,
			class_path, header->name);
	if (nmb >= sizeof(class_link)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = symlink(header->path, class_link);
	if (ret)
		ret = usbg_translate_error(errno);

	out:
	return ret;
}

static int usbg_f_uvc_set_control_class(struct usbg_f_uvc_control_desc *cd,
		struct usbg_f_uvc_control_class_desc *class)
{
	int ret;

	ret = usbg_f_uvc_link_control_class(cd->path,
			UVC_PATH_CLASS_FS, class->fs_header);
	if (ret)
		goto out;

	ret = usbg_f_uvc_link_control_class(cd->path,
			UVC_PATH_CLASS_SS, class->ss_header);
out:
	return ret;
}

static int usbg_f_uvc_set_control_header(struct usbg_f_uvc_control_desc *cd,
		struct usbg_f_uvc_control_header_desc *header)
{
	int nmb, i, ret = USBG_SUCCESS;
	char header_link[USBG_MAX_PATH_LENGTH];


	nmb = snprintf(header->path, sizeof(header->path),
			"%s" UVC_PATH_HEADER "%s", cd->path,
			header->name);
	if (nmb >= sizeof(header->path)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = uvc_create_dir(header->path);
out:
	return ret;
}

static int usbg_f_uvc_set_control(usbg_f_uvc *uvcf,
		struct usbg_f_uvc_control_desc *control)
{
	int nmb, i, ret = USBG_SUCCESS;

	nmb = snprintf(control->path, sizeof(control->path),
			"%s/%s/" UVC_PATH_CONTROL, uvcf->func.path,
			uvcf->func.name);
	if (nmb >= sizeof(control->path)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	/* Set header descriptors */
	if (control->n_header_descs <= 0 || !(*control->header))
		return USBG_ERROR_INVALID_PARAM;

	for (i = 0; i < control->n_header_descs; i++) {
		ret = usbg_f_uvc_set_control_header(control,
				control->header[i]);
		if (ret)
			return ret;
	}
	/* Set class descriptors */
	ret = usbg_f_uvc_set_control_class(control, control->class);
out:
	return ret;
}

int usbg_f_uvc_set_frame_attr_val(struct usbg_f_uvc_frame_desc *frmd, enum usbg_f_uvc_frame_attr attr,
		union usbg_f_uvc_frame_desc_val val)
{
	int ret = USBG_SUCCESS;
	if(uvc_frame_attr[attr].set)
		ret = uvc_frame_attr[attr].set(frmd->path, "", uvc_frame_attr[attr].name,
			&val);
	return ret;
}

int usbg_f_uvc_set_frame_attrs(struct usbg_f_uvc_frame_desc *frame_desc)
{
	int i, ret;

	for (i = USBG_F_UVC_FRM_ATTR_MIN; i < USBG_F_UVC_FRM_ATTR_MAX; ++i) {
			ret = usbg_f_uvc_set_frame_attr_val(frame_desc, i,
					*(union usbg_f_uvc_frame_desc_val *)((char *)frame_desc
							+ uvc_frame_attr[i].offset));
			if (ret != USBG_SUCCESS)
				break;
		}
	return ret;
}

static int usbg_f_uvc_set_streaming_frame(const char *fmtd_path,
		struct usbg_f_uvc_frame_desc *frame_desc)
{
	int nmb, i, ret = USBG_SUCCESS;


	nmb = snprintf(frame_desc->path, sizeof(frame_desc->path),
				"%s/%s", fmtd_path, frame_desc->name);
	if (nmb >= sizeof(frame_desc->path))
		return USBG_ERROR_PATH_TOO_LONG;

	ret = uvc_create_dir(frame_desc->path);
	if (ret)
		goto out;

	ret = usbg_f_uvc_set_frame_attrs(frame_desc);

out:
	return ret;
}

static int usbg_f_uvc_set_uncompressed_streaming_frame(
		struct usbg_f_uvc_u_format_desc *fmtd,
		struct usbg_f_uvc_frame_desc *frmd)
{
	return usbg_f_uvc_set_streaming_frame(fmtd->path, frmd);
}

static int usbg_f_uvc_set_mjpeg_streaming_frame(
		struct usbg_f_uvc_m_format_desc *fmtd,
		struct usbg_f_uvc_frame_desc *frmd)
{
	return usbg_f_uvc_set_streaming_frame(fmtd->path, frmd);
}

int usbg_f_uvc_set_uncompressed_format_attr_val(
		struct usbg_f_uvc_u_format_desc *fmtd,
		enum usbg_f_uvc_u_format_attr attr,
		union usbg_f_uvc_u_format_desc_val val)
{
	int ret = USBG_SUCCESS;
	if (uvc_uncompressed_format_attr[attr].set)
		ret = uvc_uncompressed_format_attr[attr].set(fmtd->path, "",
				uvc_uncompressed_format_attr[attr].name,
				&val);
	return ret;
}

int usbg_f_uvc_set_uncompressed_format_attrs(
		struct usbg_f_uvc_u_format_desc *u_fmt)
{
	int i, ret;

	for (i = USBG_F_UVC_U_FMT_ATTR_MIN;
			i < USBG_F_UVC_U_FMT_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_set_uncompressed_format_attr_val(u_fmt, i,
			*(union usbg_f_uvc_u_format_desc_val *)((char *)u_fmt + uvc_uncompressed_format_attr[i].offset));
		if (ret != USBG_SUCCESS)
			break;
	}
	return ret;
}


static int usbg_f_uvc_link_streaming_class(unsigned char *streaming_path,
		unsigned char *class_path,
		struct usbg_f_uvc_streaming_header_desc *header)
{
	char class_link[USBG_MAX_PATH_LENGTH];
	int nmb, ret;

	if (!header) {
		ret = USBG_ERROR_MISSING_TAG;
		goto out;
	}

	nmb = snprintf(class_link, sizeof(class_link), "%s%s/%s", streaming_path,
			class_path, header->name);
	if (nmb >= sizeof(class_link)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = symlink(header->path, class_link);
	if (ret)
		ret = usbg_translate_error(errno);
out:
	return ret;
}

static int usbg_f_uvc_set_streaming_class(struct usbg_f_uvc_streaming_desc *sd,
		struct usbg_f_uvc_streaming_class_desc *class)
{
	int ret;

	ret = usbg_f_uvc_link_streaming_class(sd->path,
			UVC_PATH_CLASS_FS, class->fs_header);
	if (ret)
		goto out;

	ret = usbg_f_uvc_link_streaming_class(sd->path,
			UVC_PATH_CLASS_HS, class->hs_header);
	if (ret)
		goto out;

	ret = usbg_f_uvc_link_streaming_class(sd->path,
			UVC_PATH_CLASS_SS, class->ss_header);
out:
	return ret;
}

static int usbg_f_uvc_set_streaming_header(struct usbg_f_uvc_streaming_desc *sd,
		struct usbg_f_uvc_streaming_header_desc *header)
{
	int nmb, i, ret = USBG_SUCCESS;
	char header_link[USBG_MAX_PATH_LENGTH];


	nmb = snprintf(header->path, sizeof(header->path),
			"%s" UVC_PATH_HEADER "%s", sd->path,
			header->name);
	if (nmb >= sizeof(header->path)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = uvc_create_dir(header->path);
	if (ret)
		goto out;

	if (!header->format_link){
		ret = USBG_ERROR_MISSING_TAG;
		goto out;
	}

	nmb = snprintf(header_link, sizeof(header_link), "%s/%s",
			header->path,
			header->format_link->name);
	if (nmb >= sizeof(header_link)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = symlink(header->format_link->path, header_link);
	if (ret)
		ret = usbg_translate_error(errno);
out:
	return ret;
}

static int usbg_f_uvc_set_streaming_uncompressed(struct usbg_f_uvc_streaming_desc *sd,
		struct usbg_f_uvc_u_format_desc *uncompressed_format)
{
	int nmb, i, ret = USBG_SUCCESS;

	nmb = snprintf(uncompressed_format->path, sizeof(uncompressed_format->path),
			"%s" UVC_PATH_STREAMING_UNCOMPRESSED "%s", sd->path, uncompressed_format->name);
	if (nmb >= sizeof(uncompressed_format->path)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	ret = uvc_create_dir(uncompressed_format->path);
	if (ret)
		goto out;

	ret = usbg_f_uvc_set_uncompressed_format_attrs(uncompressed_format);
	if (ret)
		goto out;

	/* Set uncompressed frame descriptors */
	if (uncompressed_format->n_frame_descs <= 0
			|| !(*uncompressed_format->frame_descs))
		return USBG_ERROR_INVALID_PARAM;

	for (i = 0; i < uncompressed_format->n_frame_descs; i++) {
		ret = usbg_f_uvc_set_uncompressed_streaming_frame(
				uncompressed_format,
				uncompressed_format->frame_descs[i]);
		if (ret)
			break;
	}
out:
	return ret;
}

static int usbg_f_uvc_set_streaming(usbg_f_uvc *uvcf, struct usbg_f_uvc_streaming_desc *sd)
{
	int nmb, i, ret = USBG_SUCCESS;

	nmb = snprintf(sd->path, sizeof(sd->path),
			"%s/%s/" UVC_PATH_STREAMING, uvcf->func.path,
			uvcf->func.name);

	if (nmb >= sizeof(sd->path)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	/* Set uncompressed format descriptors */
	if (sd->n_uncompressed_format_descs <= 0
			|| !(*sd->uncompressed))
		return USBG_ERROR_INVALID_PARAM;

	for (i = 0; i < sd->n_uncompressed_format_descs; i++) {
		ret = usbg_f_uvc_set_streaming_uncompressed(sd,
				sd->uncompressed[i]);
		if (ret)
			return ret;
	}

	/* Set header descriptors */
	if (sd->n_header_descs <= 0 || !(*sd->header))
		return USBG_ERROR_INVALID_PARAM;

	for (i = 0; i < sd->n_header_descs; i++) {
		ret = usbg_f_uvc_set_streaming_header(sd,
				sd->header[i]);
		if (ret)
			return ret;
	}

	/* Set class descriptors */
	ret = usbg_f_uvc_set_streaming_class(sd, sd->class);

out:
	return ret;
}

struct usbg_function_type usbg_f_type_uvc = {
	.name = "uvc",
	.alloc_inst = uvc_alloc_inst,
	.free_inst = uvc_free_inst,
	.set_attrs = uvc_set_attrs,
	.get_attrs = uvc_get_attrs,
	.cleanup_attrs = uvc_cleanup_attrs,
	.import = uvc_libconfig_import,
	.export = uvc_libconfig_export,
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
	int i, ret = USBG_SUCCESS;

	for (i = USBG_F_UVC_ATTR_MIN; i < USBG_F_UVC_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_get_attr_val(uvcf, i,
				(union usbg_f_uvc_attr_val *)((char *)attrs
						+ uvc_attr[i].offset));
		if(ret != USBG_SUCCESS)
			break;
	}
	return ret;
}

int usbg_f_uvc_set_attrs(usbg_f_uvc *uvcf, const struct usbg_f_uvc_attrs *attrs)
{
	int i, nmb, ret = USBG_SUCCESS;

	if(!attrs->control || !attrs->streaming)
		return USBG_ERROR_MISSING_TAG;

	ret = usbg_f_uvc_set_control(uvcf, attrs->control);
	if (ret)
		goto out;

	ret = usbg_f_uvc_set_streaming(uvcf, attrs->streaming);
	if (ret)
		goto out;


	for (i = USBG_F_UVC_ATTR_MIN; i < USBG_F_UVC_ATTR_MAX; ++i) {
		ret = usbg_f_uvc_set_attr_val(uvcf, i,
				*(union usbg_f_uvc_attr_val *)((char *)attrs
						+ uvc_attr[i].offset));
		if(ret != USBG_SUCCESS)
			break;
	}
out:
	return ret;
}

int usbg_f_uvc_get_attr_val(usbg_f_uvc *uvcf, enum usbg_f_uvc_attr attr,
		union usbg_f_uvc_attr_val *val)
{
	return uvc_attr[attr].get(uvcf->func.path, uvcf->func.name,
			uvc_attr[attr].name,
			val);
}

int usbg_f_uvc_set_attr_val(usbg_f_uvc *uvcf, enum usbg_f_uvc_attr attr,
		union usbg_f_uvc_attr_val val)
{
	return uvc_attr[attr].set(uvcf->func.path, uvcf->func.name,
			uvc_attr[attr].name,
			&val);
}
