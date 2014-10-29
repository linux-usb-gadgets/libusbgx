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

#include <usbg/usbg.h>
#include "usbg/usbg_internal.h"

int usbg_export_function(__attribute__ ((unused)) usbg_function *f,
			 __attribute__ ((unused)) FILE *stream)
{
	return USBG_ERROR_NOT_SUPPORTED;
}

int usbg_export_config(__attribute__ ((unused)) usbg_config *c,
		       __attribute__ ((unused)) FILE *stream)
{
	return USBG_ERROR_NOT_SUPPORTED;
}

int usbg_export_gadget(__attribute__ ((unused)) usbg_gadget *g,
		       __attribute__ ((unused)) FILE *stream)
{
	return USBG_ERROR_NOT_SUPPORTED;
}

int usbg_import_function(__attribute__ ((unused)) usbg_gadget *g,
			 __attribute__ ((unused)) FILE *stream,
			 __attribute__ ((unused)) const char *instance,
			 __attribute__ ((unused)) usbg_function **f)
{
	return USBG_ERROR_NOT_SUPPORTED;
}

int usbg_import_config(__attribute__ ((unused)) usbg_gadget *g,
		       __attribute__ ((unused)) FILE *stream,
		       __attribute__ ((unused)) int id,
		       __attribute__ ((unused)) usbg_config **c)
{
	return USBG_ERROR_NOT_SUPPORTED;
}

int usbg_import_gadget(__attribute__ ((unused)) usbg_state *s,
		       __attribute__ ((unused)) FILE *stream,
		       __attribute__ ((unused)) const char *name,
		       __attribute__ ((unused)) usbg_gadget **g)
{
	return USBG_ERROR_NOT_SUPPORTED;
}

const char *usbg_get_func_import_error_text(
	__attribute__ ((unused)) usbg_gadget *g)
{
	return NULL;
}

int usbg_get_func_import_error_line(__attribute__ ((unused)) usbg_gadget *g)
{
	return USBG_ERROR_NOT_SUPPORTED;
}

const char *usbg_get_config_import_error_text(
	__attribute__ ((unused)) usbg_gadget *g)
{
	return NULL;
}

int usbg_get_config_import_error_line(__attribute__ ((unused)) usbg_gadget *g)
{
	return USBG_ERROR_NOT_SUPPORTED;
}

const char *usbg_get_gadget_import_error_text(
	__attribute__ ((unused)) usbg_state *s)
{
	return NULL;
}

int usbg_get_gadget_import_error_line(__attribute__ ((unused)) usbg_state *s)
{
	return USBG_ERROR_NOT_SUPPORTED;
}
