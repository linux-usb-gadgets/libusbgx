/*
 * Copyright (C) 2024 Pengutronix
 *
 * Michael Grzeschik <m.grzeschik@pengutronix.de>
 *
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

struct usbg_f_9pfs;
typedef struct usbg_f_9pfs usbg_f_9pfs;

/**
 * @brief Convert from generic function to 9pfs
 * @param[in] f function to be converted to 9pfs.
 *         Function should be of type function fs.
 * @return Converted function or NULL if function hasn't suitable type
 */
usbg_f_9pfs *usbg_to_9pfs(usbg_function *f);

/**
 * @brief Convert form 9pfs to a generic one
 * @param[in] 9pfs function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_9pfs(usbg_f_9pfs *p9fs);

/**
 * @brief Get the device name which should be used while mounting
 *         9pfs instace into newly allocated storage
 * @param[in] pf Pointer to net function
 * @param[out] dev_name Newly allocated string containing device name
 * @return 0 on success usbg_error if error occurred.
 * @note returned dev_name should be free() by caller
 */
int usbg_f_9pfs_get_dev_name(usbg_f_9pfs *p9fs, char **dev_name);

/**
 * @brief Get the device name which should be used while mounting
 *         9pfs instace into user buffer
 * @param[in] pf Pointer to fs function
 * @param[out] buf Place where ifname should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family). This may
 *         also return negative error code from usbg_error.
 */
int usbg_f_9pfs_get_dev_name_s(usbg_f_9pfs *p9fs, char *buf, int len);

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_FS__ */
