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
extern "C"
{
#endif

#define UVC_PATH_CONTROL_HEADER       "/control/header/h"
#define UVC_PATH_CONTROL_CLASS_FS     "/control/class/fs/h"
#define UVC_PATH_CONTROL_CLASS_SS     "/control/class/ss/h"
#define UVC_PATH_STREAMING            "/streaming"
#define UVC_PATH_STREAMING_HEADER     UVC_PATH_STREAMING"/header/h"
#define UVC_PATH_STREAMING_CLASS_FS   UVC_PATH_STREAMING"/class/fs/h"
#define UVC_PATH_STREAMING_CLASS_HS   UVC_PATH_STREAMING"/class/hs/h"
#define UVC_PATH_STREAMING_CLASS_SS   UVC_PATH_STREAMING"/class/ss/h"
#define UVC_PATH_UNCOMPRESSED         "/uncompressed"
#define UVC_PATH_MJPEG                "/mjpeg"

#define MAX_FORMAT_DESCRIPTORS        4
#define MAX_FRAME_DESCRIPTORS         8

	struct usbg_f_uvc;
	typedef struct usbg_f_uvc usbg_f_uvc;

	enum usbg_f_uvc_attr_format
	{
		USBG_F_UVC_F_UNCOMPRESSED,
		USBG_F_UVC_F_MJPEG,
	};

	struct usbg_f_uvc_frame_attrs
	{
		unsigned int bmCapabilities;
		char *dwFrameInterval;
		unsigned int dwDefaultFrameInterval;
		unsigned int dwMaxVideoFrameBufferSize;
		unsigned int dwMaxBitRate;
		unsigned int dwMinBitRate;
		unsigned int wHeight;
		unsigned int wWidth;
	};

	enum usbg_f_uvc_frame_attr
	{
		USBG_F_UVC_FRAME_ATTR_MIN = 0,
		USBG_F_UVC_FRAME_ATTR_BM_CAPABILITIES = USBG_F_UVC_FRAME_ATTR_MIN,
		USBG_F_UVC_FRAME_ATTR_DW_FRAME_INTERVAL,
		USBG_F_UVC_FRAME_ATTR_DW_DEFAULT_FRAME_INTERVAL,
		USBG_F_UVC_FRAME_ATTR_DW_MAX_VIDEO_FRAME_BUFFER_SIZE,
		USBG_F_UVC_FRAME_ATTR_DW_MAX_BIT_RATE,
		USBG_F_UVC_FRAME_ATTR_DW_MIN_BIT_RATE,
		USBG_F_UVC_FRAME_ATTR_W_HEIGHT,
		USBG_F_UVC_FRAME_ATTR_W_WIDTH,
		USBG_F_UVC_FRAME_ATTR_MAX
	};

	union usbg_f_uvc_frame_attr_val
	{
		unsigned int bmCapabilities;
		char *dwFrameInterval;
		unsigned int dwDefaultFrameInterval;
		unsigned int dwMaxVideoFrameBufferSize;
		unsigned int dwMaxBitRate;
		unsigned int dwMinBitRate;
		unsigned int wHeight;
		unsigned int wWidth;
	};

	struct usbg_f_uvc_frame_descriptor
	{
		char *specific_frame_descriptor;
		struct usbg_f_uvc_frame_attrs frame_attrs;
	};

	struct usbg_f_uvc_format_desc_uncompressed
	{
		char *specific_format_descriptor;
		// ToDo
		// bmaControls   - this format's data for bmaControls in the streaming header
		// bmInterfaceFlags  - specifies interlace information, read-only
		// bAspectRatioY   - the X dimension of the picture aspect ratio, read-only
		// bAspectRatioX   - the Y dimension of the picture aspect ratio, read-only
		// bDefaultFrameIndex  - optimum frame index for this stream
		// bBitsPerPixel   - number of bits per pixel used to specify color in the decoded video frame
		// guidFormat    - globally unique id used to identify stream-encoding format
		struct usbg_f_uvc_frame_descriptor frame_descriptor[MAX_FRAME_DESCRIPTORS];
	};

	struct usbg_f_uvc_format_desc_mjpeg
	{
		char *specific_format_descriptor;
		// ToDo
		// bmaControls    - this format's data for bmaControls in the streaming header
		// bmInterfaceFlags  - specifies interlace information, read-only
		// bAspectRatioY   - the X dimension of the picture aspect ratio, read-only
		// bAspectRatioX   - the Y dimension of the picture aspect ratio, read-only
		// bmFlags     - characteristics of this format, read-only
		// bDefaultFrameIndex  - optimum frame index for this stream
		struct usbg_f_uvc_frame_descriptor frame_descriptor[MAX_FRAME_DESCRIPTORS];
	};

	struct usbg_f_uvc_format_descs
	{
		struct usbg_f_uvc_format_desc_uncompressed uncompressed[MAX_FORMAT_DESCRIPTORS];
		struct usbg_f_uvc_format_desc_mjpeg mjpeg[MAX_FORMAT_DESCRIPTORS];
	};

	struct usbg_f_uvc_attrs
	{
		unsigned int streaming_interval;
		unsigned int streaming_maxburst;
		unsigned int streaming_maxpacket;
		struct usbg_f_uvc_format_descs format_descriptors;
	};

	enum usbg_f_uvc_attr
	{
		USBG_F_UVC_ATTR_MIN = 0,
		USBG_F_UVC_STREAMING_INTERVAL = USBG_F_UVC_ATTR_MIN,
		USBG_F_UVC_STREAMING_MAX_BURST,
		USBG_F_UVC_STREAMING_MAX_PACKET,
		USBG_F_UVC_FORMAT_DESCS,
		USBG_F_UVC_ATTR_MAX
	};

	union usbg_f_uvc_attr_val
	{
		unsigned int streaming_interval;
		unsigned int streaming_maxburst;
		unsigned int streaming_maxpacket;
		struct usbg_f_uvc_format_descs format_descriptors;
	};

	usbg_f_uvc *usbg_to_uvc_function(usbg_function *f);
	usbg_function *usbg_from_uvc_function(usbg_f_uvc *ff);
	int usbg_f_uvc_get_attrs(usbg_f_uvc *af, struct usbg_f_uvc_attrs *attrs);
	int usbg_f_uvc_set_attrs(usbg_f_uvc *af, const struct usbg_f_uvc_attrs *attrs);

	static inline void usbg_f_uvc_cleanup_attrs(struct usbg_f_uvc_attrs *attrs)
	{
	}

	int usbg_f_uvc_get_attr_val(usbg_f_uvc *af, enum usbg_f_uvc_attr attr,
			union usbg_f_uvc_attr_val *val);
	int usbg_f_uvc_set_attr_val(usbg_f_uvc *af, enum usbg_f_uvc_attr attr,
			union usbg_f_uvc_attr_val val);

	int usbg_f_uvc_set_format_descs(const char *path, const char *name,
			const char *attr, void *val);
	int usbg_f_uvc_get_format_descs(const char *path, const char *name,
			const char *attr, void *val);

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_UVC__ */
