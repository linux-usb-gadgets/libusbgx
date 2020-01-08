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

enum uvc_format
{
	UVC_FORMAT_MJPEG,
	UVC_FORMAT_UNCOMPRESSED
};

struct usbg_f_uvc_format_attrs
{
	enum uvc_format format;
	const char *dwFrameInterval;
	int height;
	int width;
};

struct usbg_f_uvc_attrs
{
	struct usbg_f_uvc_format_attrs **formats;
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
	struct usbg_f_uvc_format_attrs **format_attrs;
	int i;

	if (attrs) {
		for(format_attrs = attrs->formats, i = 0; format_attrs[i]; ++i) {
			if (format_attrs[i]) {
				free((char *)format_attrs[i]->dwFrameInterval);
				format_attrs[i]->dwFrameInterval = NULL;
			}
		}
	}
}

int usbg_f_uvc_get_attrs(usbg_f_uvc *uvcf, struct usbg_f_uvc_attrs *attrs);
int usbg_f_uvc_set_attrs(usbg_f_uvc *uvcf, const struct usbg_f_uvc_attrs *attrs);


#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_UVC__ */
