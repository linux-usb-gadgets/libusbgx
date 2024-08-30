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

struct usbg_f_uvc;
typedef struct usbg_f_uvc usbg_f_uvc;

struct usbg_f_uvc_config_attrs
{
	int streaming_maxburst;
	int streaming_maxpacket;
	int streaming_interval;
	const char *function_name;
};

struct usbg_f_uvc_frame_attrs
{
	int bFrameIndex;
	int bmCapabilities;
	int dwMinBitRate;
	int dwMaxBitRate;
	int dwMaxVideoFrameBufferSize;
	int dwDefaultFrameInterval;
	int dwFrameInterval;
	int wWidth;
	int wHeight;
};

struct usbg_f_uvc_format_attrs
{
	int bmaControls;
	int bFormatIndex;
	int bDefaultFrameIndex;
	int bAspectRatioX;
	int bAspectRatioY;
	int bmInterlaceFlags;
	const char *format;
	int bBitsPerPixel;
	const char * guidFormat;
	struct usbg_f_uvc_frame_attrs **frames;
};

struct usbg_f_uvc_attrs
{
	struct usbg_f_uvc_format_attrs **formats;
};

enum usbg_f_uvc_config_attr {
	USBG_F_UVC_CONFIG_ATTR_MIN = 0,
	USBG_F_UVC_CONFIG_MAXBURST = USBG_F_UVC_CONFIG_ATTR_MIN,
	USBG_F_UVC_CONFIG_MAXPACKET,
	USBG_F_UVC_CONFIG_INTERVAL,
	USBG_F_UVC_CONFIG_FUNCTION_NAME,
	USBG_F_UVC_CONFIG_ATTR_MAX
};

enum usbg_f_uvc_frame_attr {
	USBG_F_UVC_FRAME_ATTR_MIN = 0,
	USBG_F_UVC_FRAME_INDEX = USBG_F_UVC_FRAME_ATTR_MIN,
	USBG_F_UVC_FRAME_CAPABILITIES,
	USBG_F_UVC_FRAME_MIN_BITRATE,
	USBG_F_UVC_FRAME_MAX_BITRATE,
	USBG_F_UVC_FRAME_MAX_VIDEO_BUFFERSIZE,
	USBG_F_UVC_FRAME_DEFAULT_INTERVAL,
	USBG_F_UVC_FRAME_INTERVAL,
	USBG_F_UVC_FRAME_HEIGHT,
	USBG_F_UVC_FRAME_WIDTH,
	USBG_F_UVC_FRAME_ATTR_MAX
};

enum usbg_f_uvc_format_attr {
	USBG_F_UVC_FORMAT_ATTR_MIN = 0,
	USBG_F_UVC_FORMAT_CONTROLS = USBG_F_UVC_FORMAT_ATTR_MIN,
	USBG_F_UVC_FORMAT_INTERLACE_FLAGS,
	USBG_F_UVC_FORMAT_ASPECTRATIO_Y,
	USBG_F_UVC_FORMAT_ASPECTRATIO_X,
	USBG_F_UVC_FORMAT_DEFAULT_FRAME_INDEX,
	USBG_F_UVC_FORMAT_FORMAT_INDEX,
	USBG_F_UVC_FORMAT_BITS_PER_PIXEL,
	USBG_F_UVC_FORMAT_GUID_FORMAT,
	USBG_F_UVC_FORMAT_ATTR_MAX
};

union usbg_f_uvc_config_attr_val {
	int streaming_maxburst;
	int streaming_maxpacket;
	int streaming_interval;
	const char *function_name;
};

union usbg_f_uvc_frame_attr_val {
	int bmCapabilities;
	int dwMinBitRate;
	int dwMaxBitRate;
	int dwMaxVideoFrameBufferSize;
	int dwDefaultFrameInterval;
	int dwFrameInterval;
	int wWidth;
	int wHeight;
};

union usbg_f_uvc_format_attr_val {
	int bmaControls;
	int bFormatIndex;
	int bDefaultFrameIndex;
	int bAspectRatioX;
	int bAspectRatioY;
	int bmInterlaceFlags;
	int bBitsPerPixel;
	const char * guidFormat;
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

/**
 * @brief Cleanup attributes structure after usage
 * @param[in] attrs to be cleaned up
 */
static inline void usbg_f_uvc_cleanup_attrs(struct usbg_f_uvc_attrs *attrs)
{
}

int usbg_f_uvc_get_attrs(usbg_f_uvc *uvcf, struct usbg_f_uvc_attrs *attrs);
int usbg_f_uvc_set_attrs(usbg_f_uvc *uvcf, const struct usbg_f_uvc_attrs *attrs);


#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_UVC__ */
