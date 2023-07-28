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

#ifndef USBG_FUNCTION_PRINTER__
#define USBG_FUNCTION_PRINTER__

#include <usbg/usbg.h>

#include <malloc.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct usbg_f_printer;
typedef struct usbg_f_printer usbg_f_printer;

struct usbg_f_printer_attrs {
	const char *pnp_string;
	int q_len;
};

enum usbg_f_printer_attr {
	USBG_F_PRINTER_ATTR_MIN = 0,
	USBG_F_PRINTER_PNP_STRING = USBG_F_PRINTER_ATTR_MIN,
	USBG_F_PRINTER_Q_LEN,
	USBG_F_PRINTER_ATTR_MAX
};

union usbg_f_printer_attr_val {
	const char *pnp_string;
	int q_len;
};

#define USBG_F_PRINTER_ETHER_ADDR_TO_ATTR_VAL(WHAT) \
	USBG_TO_UNION(usbg_f_printer_attr_val, dev_addr, WHAT)

#define USBG_F_PRINTER_INT_TO_ATTR_VAL(WHAT) \
	USBG_TO_UNION(usbg_f_printer_attr_val, qmult, WHAT)


#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_PRINTER__ */
