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

#ifndef USBG_FUNCTION_PHONET__
#define USBG_FUNCTION_PHONET__

#include <usbg/usbg.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usbg_f_phonet;
typedef struct usbg_f_phonet usbg_f_phonet;

/**
 * @brief Convert from generic function to phonet function
 * @param[in] f function to be converted to phonet funciton.
 *         Function should be of type phonet.
 * @return Converted phonet function or NULL if function hasn't suitable type
 */
usbg_f_phonet *usbg_to_phonet_function(usbg_function *f);

/**
 * @brief Convert form phonet function to generic one
 * @param[in] pf function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_phonet_function(usbg_f_phonet *pf);

/**
 * @brief Get the value of interface name associated with this function
 *         into newly allocated storage
 * @param[in] pf Pointer to net function
 * @param[out] ifname Newly allocated string containing name of interface
 * @return 0 on success usbg_error if error occurred.
 * @note returned ifname should be free() by caller
 */
int usbg_f_phonet_get_ifname(usbg_f_phonet *pf, char **ifname);

/**
 * @brief Get the value of interface name associated with this function
 *         into user buffer
 * @param[in] pf Pointer to phonet function
 * @param[out] buf Place where ifname should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family). This may
 *         also return negative error code from usbg_error.
 */
int usbg_f_phonet_get_ifname_s(usbg_f_phonet *pf, char *buf, int len);

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_PHONET__ */
