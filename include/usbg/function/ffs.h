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

#ifndef USBG_FUNCTION_FS__
#define USBG_FUNCTION_FS__

#include <usbg/usbg.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usbg_f_fs;
typedef struct usbg_f_fs usbg_f_fs;

/**
 * @brief Convert from generic function to fs function
 * @param[in] f function to be converted to fs funciton.
 *         Function should be of type function fs.
 * @return Converted function or NULL if function hasn't suitable type
 */
usbg_f_fs *usbg_to_fs_function(usbg_function *f);

/**
 * @brief Convert form fs function to a generic one
 * @param[in] pf function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_fs_function(usbg_f_fs *ff);

/**
 * @brief Get the device name which should be used while mounting
 *         functionfs instace into newly allocated storage
 * @param[in] pf Pointer to net function
 * @param[out] dev_name Newly allocated string containing device name
 * @return 0 on success usbg_error if error occurred.
 * @note returned dev_name should be free() by caller
 */
int usbg_f_fs_get_dev_name(usbg_f_fs *ff, char **dev_name);

/**
 * @brief Get the device name which should be used while mounting
 *         functionfs instace into user buffer
 * @param[in] pf Pointer to fs function
 * @param[out] buf Place where ifname should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family). This may
 *         also return negative error code from usbg_error.
 */
int usbg_f_fs_get_dev_name_s(usbg_f_fs *ff, char *buf, int len);

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_FS__ */
