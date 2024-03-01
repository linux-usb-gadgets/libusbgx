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

#include "usbg/usbg.h"
#include "usbg/usbg_internal.h"

#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

struct usbg_f_printer {
	struct usbg_function func;
};

GENERIC_ALLOC_INST(printer, struct usbg_f_printer, func)

GENERIC_FREE_INST(printer, struct usbg_f_printer, func)

#define PRINTER_FUNCTION_OPTS			\
	.alloc_inst = printer_alloc_inst,	\
	.free_inst = printer_free_inst

struct usbg_function_type usbg_f_type_printer = {
    .name = "printer",
    PRINTER_FUNCTION_OPTS};
