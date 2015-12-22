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

#include <errno.h>

int usbg_translate_error(int error)
{
	int ret;

	switch (error) {
	case ENOMEM:
		ret = USBG_ERROR_NO_MEM;
		break;
	case EACCES:
	case EROFS:
	case EPERM:
		ret = USBG_ERROR_NO_ACCESS;
		break;
	case ENOENT:
	case ENOTDIR:
		ret = USBG_ERROR_NOT_FOUND;
		break;
	case ERANGE:
	case EINVAL:
	case USBG_ERROR_INVALID_PARAM:
		ret = USBG_ERROR_INVALID_PARAM;
		break;
	case EIO:
		ret = USBG_ERROR_IO;
		break;
	case EEXIST:
		ret = USBG_ERROR_EXIST;
		break;
	case ENODEV:
		ret = USBG_ERROR_NO_DEV;
		break;
	case EBUSY:
		ret = USBG_ERROR_BUSY;
		break;
	case ENOTEMPTY:
		ret = USBG_ERROR_NOT_EMPTY;
		break;
	default:
		ret = USBG_ERROR_OTHER_ERROR;
	}

	return ret;
}

const char *usbg_error_name(usbg_error e)
{
	char *ret = "UNKNOWN";

	switch (e) {
	case USBG_SUCCESS:
		ret = "USBG_SUCCESS";
		break;
	case USBG_ERROR_NO_MEM:
		ret = "USBG_ERROR_NO_MEM";
		break;
	case USBG_ERROR_NO_ACCESS:
		ret = "USBG_ERROR_NO_ACCESS";
		break;
	case USBG_ERROR_INVALID_PARAM:
		ret = "USBG_ERROR_INVALID_PARAM";
		break;
	case USBG_ERROR_NOT_FOUND:
		ret = "USBG_ERROR_NOT_FOUND";
		break;
	case USBG_ERROR_IO:
		ret = "USBG_ERROR_IO";
		break;
	case USBG_ERROR_EXIST:
		ret = "USBG_ERROR_EXIST";
		break;
	case USBG_ERROR_NO_DEV:
		ret = "USBG_ERROR_NO_DEV";
		break;
	case USBG_ERROR_BUSY:
		ret = "USBG_ERROR_BUSY";
		break;
	case USBG_ERROR_NOT_SUPPORTED:
		ret = "USBG_ERROR_NOT_SUPPORTED";
		break;
	case USBG_ERROR_PATH_TOO_LONG:
		ret = "USBG_ERROR_PATH_TOO_LONG";
		break;
	case USBG_ERROR_INVALID_FORMAT:
		ret = "USBG_ERROR_INVALID_FORMAT";
		break;
	case USBG_ERROR_MISSING_TAG:
		ret = "USBG_ERROR_MISSING_TAG";
		break;
	case USBG_ERROR_INVALID_TYPE:
		ret = "USBG_ERROR_INVALID_TYPE";
		break;
	case USBG_ERROR_INVALID_VALUE:
		ret = "USBG_ERROR_INVALID_VALUE";
		break;
	case USBG_ERROR_NOT_EMPTY:
		ret = "USBG_ERROR_NOT_EMPTY";
		break;
	case USBG_ERROR_OTHER_ERROR:
		ret = "USBG_ERROR_OTHER_ERROR";
		break;
	}

	return ret;
}

const char *usbg_strerror(usbg_error e)
{
	char *ret = "Unknown error";

	switch (e) {
	case USBG_SUCCESS:
		ret = "Success";
		break;
	case USBG_ERROR_NO_MEM:
		ret = "Insufficient memory";
		break;
	case USBG_ERROR_NO_ACCESS:
		ret = "Access denied (insufficient permissions)";
		break;
	case USBG_ERROR_INVALID_PARAM:
		ret = "Invalid parameter";
		break;
	case USBG_ERROR_NOT_FOUND:
		ret = "Not found (file or directory removed)";
		break;
	case USBG_ERROR_IO:
		ret = "Input/output error";
		break;
	case USBG_ERROR_EXIST:
		ret = "Already exist";
		break;
	case USBG_ERROR_NO_DEV:
		ret = "No such device (illegal device name)";
		break;
	case USBG_ERROR_BUSY:
		ret = "Busy (gadget enabled)";
		break;
	case USBG_ERROR_NOT_SUPPORTED:
		ret = "Function not supported";
		break;
	case USBG_ERROR_PATH_TOO_LONG:
		ret = "Created path was too long to process it.";
		break;
	case USBG_ERROR_INVALID_FORMAT:
		ret = "Given file has incompatible format.";
		break;
	case USBG_ERROR_MISSING_TAG:
		ret = "One of mandatory tags is missing.";
		break;
	case USBG_ERROR_INVALID_TYPE:
		ret = "One of attributes has incompatible type.";
		break;
	case USBG_ERROR_INVALID_VALUE:
		ret = "Incorrect value provided as attribute.";
		break;
	case USBG_ERROR_NOT_EMPTY:
		ret = "Entity is not empty.";
		break;
	case USBG_ERROR_OTHER_ERROR:
		ret = "Other error";
		break;
	}

	return ret;
}
