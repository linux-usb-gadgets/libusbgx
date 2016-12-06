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

#ifndef USBG_FUNCTION_SERIAL__
#define USBG_FUNCTION_SERIAL__

#include <usbg/usbg.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usbg_f_serial;
typedef struct usbg_f_serial usbg_f_serial;

/**
 * @brief Convert from generic function to serial function
 * @param[in] f function to be converted to serial funciton.
 *         Function should be one of types: serial, ACM, OBEX.
 * @return Converted serial function or NULL if function hasn't suitable type
 */
usbg_f_serial *usbg_to_serial_function(usbg_function *f);

/**
 * @brief Convert form serial function to generic one
 * @param[in] sf function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_serial_function(usbg_f_serial *sf);

/**
 * @brief Get the id of device side tty port (/dev/ttyGS<port_num>)
 * @param[in] sf Pointer to serial function
 * @param[out] port_num the id of device side tty port
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_serial_get_port_num(usbg_f_serial *sf, int *port_num);

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_SERIAL__ */
