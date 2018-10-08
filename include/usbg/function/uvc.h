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

#ifndef USBG_FUNCTION_UVC__
#define USBG_FUNCTION_UVC__

#include <usbg/usbg.h>

#ifdef __cplusplus
extern "C" {
#endif


#define USBG_UVC_MAX_PATH_LENGTH 1024

struct usbg_f_uvc;
typedef struct usbg_f_uvc usbg_f_uvc;

struct usbg_f_uvc_frame_desc
{
	char path[USBG_UVC_MAX_PATH_LENGTH];
	const char *name;
	unsigned int bFrameIndex;
	char *dwFrameInterval;
	unsigned int dwDefaultFrameInterval;
	unsigned int dwMaxVideoFrameBufferSize;
	unsigned int dwMaxBitRate;
	unsigned int dwMinBitRate;
	unsigned int wHeight;
	unsigned int wWidth;
	unsigned int bmCapabilities;
};

enum usbg_f_uvc_frame_attr
{
	USBG_F_UVC_FRM_ATTR_MIN = 0,
	USBG_F_UVC_FRM_ATTR_B_FRAME_INDEX = USBG_F_UVC_FRM_ATTR_MIN,
	USBG_F_UVC_FRM_ATTR_DW_FRAME_INTERVAL,
	USBG_F_UVC_FRM_ATTR_DW_DEFAULT_FRAME_INTERVAL,
	USBG_F_UVC_FRM_ATTR_DW_MAX_VIDEO_FRAME_BUFFER_SIZE,
	USBG_F_UVC_FRM_ATTR_DW_MAX_BIT_RATE,
	USBG_F_UVC_FRM_ATTR_DW_MIN_BIT_RATE,
	USBG_F_UVC_FRM_ATTR_W_HEIGHT,
	USBG_F_UVC_FRM_ATTR_W_WIDTH,
	USBG_F_UVC_FRM_ATTR_BM_CAPABILITIES,
	USBG_F_UVC_FRM_ATTR_MAX
};

union usbg_f_uvc_frame_desc_val
{
	unsigned int bFrameIndex;
	char *dwFrameInterval;
	unsigned int dwDefaultFrameInterval;
	unsigned int dwMaxVideoFrameBufferSize;
	unsigned int dwMaxBitRate;
	unsigned int dwMinBitRate;
	unsigned int wHeight;
	unsigned int wWidth;
	unsigned int bmCapabilities;
};

struct usbg_f_uvc_streaming_header_desc
{
	char path[USBG_UVC_MAX_PATH_LENGTH];
	const char *name;
	struct usbg_f_uvc_u_format_desc *format_link;
};

struct usbg_f_uvc_u_format_desc
{
	char path[USBG_UVC_MAX_PATH_LENGTH];
	const char *name;
	unsigned int bFormatIndex;
	unsigned int bmaControls;
	unsigned int bmInterfaceFlags;
	unsigned int bAspectRatioY;
	unsigned int bAspectRatioX;
	unsigned int bDefaultFrameIndex;
	unsigned int bBitsPerPixel;
	char guidFormat[16];
	int n_frame_descs;
	struct usbg_f_uvc_frame_desc **frame_descs;
};

enum usbg_f_uvc_u_format_attr
{
	USBG_F_UVC_U_FMT_ATTR_MIN = 0,
	USBG_F_UVC_U_FMT_ATTR_B_FORMAT_INDEX = USBG_F_UVC_U_FMT_ATTR_MIN,
	USBG_F_UVC_U_FMT_ATTR_BMA_CONTROLS,
	USBG_F_UVC_U_FMT_ATTR_BM_INTERFACE_FLAGS,
	USBG_F_UVC_U_FMT_ATTR_B_ASPECT_RATIO_Y,
	USBG_F_UVC_U_FMT_ATTR_B_ASPECT_RATIO_X,
	USBG_F_UVC_U_FMT_ATTR_B_DEFAULT_FRAME_INDEX,
	USBG_F_UVC_U_FMT_ATTR_B_BITS_PER_PIXEL,
	USBG_F_UVC_U_FMT_ATTR_GUID_FORMAT,
	USBG_F_UVC_U_FMT_ATTR_MAX
};

union usbg_f_uvc_u_format_desc_val
{
	unsigned int bFormatIndex;
	unsigned int bmaControls;
	unsigned int bmInterfaceFlags;
	unsigned int bAspectRatioY;
	unsigned int bAspectRatioX;
	unsigned int bDefaultFrameIndex;
	unsigned int bBitsPerPixel;
	char *guidFormat;
};

struct usbg_f_uvc_m_format_desc
{
	char path[USBG_UVC_MAX_PATH_LENGTH];
	const char *name;
	// ToDo
	// bmaControls    - this format's data for bmaControls in the streaming header
	// bmInterfaceFlags  - specifies interlace information, read-only
	// bAspectRatioY   - the X dimension of the picture aspect ratio, read-only
	// bAspectRatioX   - the Y dimension of the picture aspect ratio, read-only
	// bmFlags     - characteristics of this format, read-only
	// bDefaultFrameIndex  - optimum frame index for this stream
	int n_frame_descs;
	struct usbg_f_uvc_frame_desc **frame_descs;
};

struct usbg_f_uvc_color_matching_desc
{
	const char *name;
};

struct usbg_f_uvc_streaming_class_desc
{
	struct usbg_f_uvc_streaming_header_desc *ss_header;
	struct usbg_f_uvc_streaming_header_desc *hs_header;
	struct usbg_f_uvc_streaming_header_desc *fs_header;
};

struct usbg_f_uvc_control_header_desc
{
	char path[USBG_UVC_MAX_PATH_LENGTH];
	const char *name;
};

struct usbg_f_uvc_processing_desc
{
	const char *name;
};

struct usbg_f_uvc_camera_terminal_desc
{
	const char *name;
};

struct usbg_f_uvc_output_terminal_desc
{
	const char *name;
	unsigned int iTerminal;
	unsigned int bSourceID;
	unsigned int bAssocTerminal;
	unsigned int wTerminalType;
	unsigned int bTerminalID;
};

struct usbg_f_uvc_terminal_desc
{
	struct usbg_f_uvc_output_terminal_desc **output;
	struct usbg_f_uvc_camera_terminal_desc **camera;
};

struct usbg_f_uvc_control_class_desc
{
	struct usbg_f_uvc_control_header_desc *ss_header;
	struct usbg_f_uvc_control_header_desc *fs_header;
};

struct usbg_f_uvc_streaming_desc
{
	char path[USBG_UVC_MAX_PATH_LENGTH];
	unsigned int bInterfaceNumber;
	struct usbg_f_uvc_streaming_class_desc *class;
	int n_color_matching_descs;
	struct usbg_f_uvc_color_matching_desc **color_matching;
	int n_mjpeg_format_descs;
	struct usbg_f_uvc_m_format_desc **mjpeg;
	int n_uncompressed_format_descs;
	struct usbg_f_uvc_u_format_desc **uncompressed;
	int n_header_descs;
	struct usbg_f_uvc_streaming_header_desc **header;
};

struct usbg_f_uvc_control_desc
{
	char path[USBG_UVC_MAX_PATH_LENGTH];
	unsigned int bInterfaceNumber;
	struct usbg_f_uvc_control_class_desc *class;
	struct usbg_f_uvc_terminal_desc *terminal;
	int n_processing_descs;
	struct usbg_f_uvc_processing_desc **processing;
	int n_header_descs;
	struct usbg_f_uvc_control_header_desc **header;
};

struct usbg_f_uvc_attrs
{
	unsigned int streaming_interval;
	unsigned int streaming_maxburst;
	unsigned int streaming_maxpacket;
	struct usbg_f_uvc_control_desc *control;
	struct usbg_f_uvc_streaming_desc *streaming;
};

enum usbg_f_uvc_attr
{
	USBG_F_UVC_ATTR_MIN = 0,
	USBG_F_UVC_STREAMING_INTERVAL = USBG_F_UVC_ATTR_MIN,
	USBG_F_UVC_STREAMING_MAX_BURST,
	USBG_F_UVC_STREAMING_MAX_PACKET,
	USBG_F_UVC_ATTR_MAX
};

union usbg_f_uvc_attr_val
{
	unsigned int streaming_interval;
	unsigned int streaming_maxburst;
	unsigned int streaming_maxpacket;
};

/**
 * @brief Cast from generic function to uvc function
 * @param[in] f function to be converted to uvc funciton.
 *         Should be one of types:
 *         ecm, subset, ncm, eem, rndis
 * @return Converted uvc function or NULL if function hasn't suitable type
 */
usbg_f_uvc *usbg_to_uvc_function(usbg_function *f);

/**
 * @brief Cast form uvc function to generic one
 * @param[in] uvc function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_uvc_function(usbg_f_uvc *ff);

int usbg_f_uvc_get_attrs(usbg_f_uvc *uvcf, struct usbg_f_uvc_attrs *attrs);
int usbg_f_uvc_set_attrs(usbg_f_uvc *uvcf, const struct usbg_f_uvc_attrs *attrs);

static inline void usbg_f_uvc_cleanup_attrs(struct usbg_f_uvc_attrs *attrs)
{
}

int usbg_f_uvc_get_attr_val(usbg_f_uvc *uvcf, enum usbg_f_uvc_attr attr,
		union usbg_f_uvc_attr_val *val);
int usbg_f_uvc_set_attr_val(usbg_f_uvc *uvcf, enum usbg_f_uvc_attr attr,
		union usbg_f_uvc_attr_val val);

int usbg_f_uvc_set_frame_attr_val(struct usbg_f_uvc_frame_desc *parent,
		enum usbg_f_uvc_frame_attr attr,
		union usbg_f_uvc_frame_desc_val val);
int usbg_f_uvc_set_uncompressed_format_attr_val(
		struct usbg_f_uvc_u_format_desc *parent,
		enum usbg_f_uvc_u_format_attr attr,
		union usbg_f_uvc_u_format_desc_val val);

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_UVC__ */
