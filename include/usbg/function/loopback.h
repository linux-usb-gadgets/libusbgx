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

#ifndef USBG_FUNCTION_LOOPBACK__
#define USBG_FUNCTION_LOOPBACK__

#include <usbg/usbg.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usbg_f_loopback;
typedef struct usbg_f_loopback usbg_f_loopback;

struct usbg_f_loopback_attrs {
	unsigned int buflen;
	unsigned int qlen;
};

enum usbg_f_loopback_attr {
	USBG_F_LOOPBACK_ATTR_MIN = 0,
	USBG_F_LOOPBACK_BUFLEN = USBG_F_LOOPBACK_ATTR_MIN,
	USBG_F_LOOPBACK_QLEN,
	USBG_F_LOOPBACK_ATTR_MAX
};

/**
 * @brief Cast from generic function to loopback function
 * @param[in] f function to be converted to loopback funciton.
 *         Function should be of type loopback.
 * @return Converted loopback function or NULL if function hasn't suitable type
 */
usbg_f_loopback *usbg_to_loopback_function(usbg_function *f);

/**
 * @brief Cast form loopback function to generic one
 * @param[in] lf function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_loopback_function(usbg_f_loopback *lf);

/**
 * @brief Get attributes of given loopback function
 * @param[in] lf Pointer to loopback function
 * @param[out] attrs Structure to be filled with data
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_loopback_get_attrs(usbg_f_loopback *lf,
			      struct usbg_f_loopback_attrs *attrs);

/**
 * @brief Set attributes of given loopback function
 * @param[in] lf Pointer to loopback function
 * @param[in] attrs to be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_loopback_set_attrs(usbg_f_loopback *lf,
			 const struct usbg_f_loopback_attrs *attrs);

/**
 * @brief Get the value of single attribute
 * @param[in] lf Pointer to loopback function
 * @param[in] attr Code of attribute which value should be taken
 * @param[out] val Current value of this attribute
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_loopback_get_attr_val(usbg_f_loopback *lf,
				 enum usbg_f_loopback_attr attr, int *val);

/**
 * @brief Set the value of single attribute
 * @param[in] lf Pointer to loopback function
 * @param[in] attr Code of attribute which value should be set
 * @param[in] val Value of attribute which should be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_loopback_set_attr_val(usbg_f_loopback *lf,
				 enum usbg_f_loopback_attr attr, int val);

/**
 * @brief Get the size of request buffer
 * @details This is the maximum number of bytes which can be received
 *        using single usb_request.
 * @param[in] lf Pointer to loopback function
 * @param[out] buflen size of request buffer
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_loopback_get_buflen(usbg_f_loopback *lf, int *buflen)
{
	return usbg_f_loopback_get_attr_val(lf, USBG_F_LOOPBACK_BUFLEN, buflen);
}

/**
 * @brief Set the size of request buffer
 * @details This is the maximum number of bytes which can be received
 *        using single usb_request.
 * @param[in] lf Pointer to loopback function
 * @param[in] buflen size of request buffer
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_loopback_set_buflen(usbg_f_loopback *lf, int buflen)
{
	return usbg_f_loopback_set_attr_val(lf, USBG_F_LOOPBACK_BUFLEN, buflen);
}

/**
 * @brief Get the value of request queue length
 * @details This length is the number of OUT requests which can be received
 *         by function without requesting any IN transfers by host
 * @param[in] lf Pointer to loopback function
 * @param[out] qlen Current queue length
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_loopback_get_qlen(usbg_f_loopback *lf, int *qlen)
{
	return usbg_f_loopback_get_attr_val(lf, USBG_F_LOOPBACK_QLEN, qlen);
}

/**
 * @brief Set the value of request queue length
 * @details This length is the number of OUT requests which can be received
 *         by function without requesting any IN transfers by host
 * @param[in] lf Pointer to loopback function
 * @param[in] qlen Current queue length
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_loopback_set_qlen(usbg_f_loopback *lf, int qlen)
{
	return usbg_f_loopback_set_attr_val(lf, USBG_F_LOOPBACK_QLEN, qlen);
}

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_LOOPBACK__ */
