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
#include <sys/stat.h>
#include <sys/types.h>
#include <malloc.h>

int usbg_read_buf_limited(const char *path, const char *name,
			  const char *file, char *buf, int len)
{
	char p[USBG_MAX_PATH_LENGTH];
	FILE *fp;
	char *ret_ptr;
	int nmb;
	int ret = USBG_SUCCESS;

	nmb = snprintf(p, sizeof(p), "%s/%s/%s", path, name, file);
	if (nmb >= sizeof(p)) {
		ret = USBG_ERROR_PATH_TOO_LONG;
		goto out;
	}

	fp = fopen(p, "r");
	if (!fp) {
		/* Set error correctly */
		ret = usbg_translate_error(errno);
		goto out;
	}

	ret_ptr = fgets(buf, len, fp);
	if (!ret_ptr) {
		/* File is empty */
		if (feof(fp))
			buf[0] = '\0';
		/* Error occurred */
		else
			ret = USBG_ERROR_IO;
	}

	fclose(fp);

out:
	return ret;
}

int usbg_read_buf(const char *path, const char *name,
		  const char *file, char *buf)
{
	return usbg_read_buf_limited(path, name, file, buf, USBG_MAX_STR_LENGTH);
}

int usbg_read_int(const char *path, const char *name, const char *file,
			 int base, int *dest)
{
	char buf[USBG_MAX_STR_LENGTH];
	char *pos;
	int ret;

	ret = usbg_read_buf(path, name, file, buf);
	if (ret == USBG_SUCCESS) {
		*dest = strtol(buf, &pos, base);
		if (!pos)
			ret = USBG_ERROR_OTHER_ERROR;
	}

	return ret;
}

int usbg_read_bool(const char *path, const char *name,
		   const char *file,  bool *dest)
{
	int buf;
	int ret;

	ret = usbg_read_dec(path, name, file, &buf);
	if (ret != USBG_SUCCESS)
		goto out;

	*dest = !!buf;
out:
	return ret;
}

int usbg_read_string(const char *path, const char *name,
		     const char *file, char *buf)
{
	return usbg_read_string_limited(path, name, file, buf,
					USBG_MAX_STR_LENGTH);
}

int usbg_read_string_limited(const char *path, const char *name,
			     const char *file, char *buf, int len)
{
	char *p = NULL;
	int ret;

	ret = usbg_read_buf_limited(path, name, file, buf, len);
	/* Check whether read was successful */
	if (ret == USBG_SUCCESS) {
		if ((p = strchr(buf, '\n')) != NULL)
				*p = '\0';
	} else {
		/* Set this as empty string */
		*buf = '\0';
	}

	return ret;

}

int usbg_read_string_alloc(const char *path, const char *name,
			   const char *file, char **dest)
{
	char buf[USBG_MAX_FILE_SIZE];
	char *new_buf = NULL;
	int ret = USBG_SUCCESS;

	ret = usbg_read_string(path, name, file, buf);
	if (ret != USBG_SUCCESS)
		goto out;

	new_buf = strdup(buf);
	if (!new_buf) {
		ret = USBG_ERROR_NO_MEM;
		goto out;
	}

	*dest = new_buf;
out:
	return ret;
}

int usbg_write_buf(const char *path, const char *name,
		   const char *file, const char *buf)
{
	char p[USBG_MAX_PATH_LENGTH];
	FILE *fp;
	int nmb;
	int ret = USBG_SUCCESS;

	nmb = snprintf(p, sizeof(p), "%s/%s/%s", path, name, file);
	if (nmb < sizeof(p)) {
		fp = fopen(p, "w");
		if (fp) {
			fputs(buf, fp);
			fflush(fp);

			ret = ferror(fp);
			if (ret)
				ret = usbg_translate_error(errno);

			fclose(fp);
		} else {
			/* Set error correctly */
			ret = usbg_translate_error(errno);
		}
	} else {
		ret = USBG_ERROR_PATH_TOO_LONG;
	}

	return ret;
}

int usbg_write_int(const char *path, const char *name, const char *file,
		   int value, const char *str)
{
	char buf[USBG_MAX_STR_LENGTH];
	int nmb;

	nmb = snprintf(buf, USBG_MAX_STR_LENGTH, str, value);
	return nmb < USBG_MAX_STR_LENGTH ?
			usbg_write_buf(path, name, file, buf)
			: USBG_ERROR_INVALID_PARAM;
}

int usbg_write_string(const char *path, const char *name,
		      const char *file, const char *buf)
{
	return usbg_write_buf(path, name, file, buf);
}

int ubsg_rm_file(const char *path, const char *name)
{
	int ret = USBG_SUCCESS;
	int nmb;
	char buf[USBG_MAX_PATH_LENGTH];

	nmb = snprintf(buf, sizeof(buf), "%s/%s", path, name);
	if (nmb < sizeof(buf)) {
		nmb = unlink(buf);
		if (nmb != 0)
			ret = usbg_translate_error(errno);
	} else {
		ret = USBG_ERROR_PATH_TOO_LONG;
	}

	return ret;
}

int usbg_rm_dir(const char *path, const char *name)
{
	int ret = USBG_SUCCESS;
	int nmb;
	char buf[USBG_MAX_PATH_LENGTH];

	nmb = snprintf(buf, sizeof(buf), "%s/%s", path, name);
	if (nmb < sizeof(buf)) {
		nmb = rmdir(buf);
		if (nmb != 0)
			ret = usbg_translate_error(errno);
	} else {
		ret = USBG_ERROR_PATH_TOO_LONG;
	}

	return ret;
}

int usbg_rm_all_dirs(const char *path)
{
	int ret = USBG_SUCCESS;
	int n, i;
	struct dirent **dent;

	n = scandir(path, &dent, file_select, alphasort);
	if (n >= 0) {
		for (i = 0; i < n; ++i) {
			if (ret == USBG_SUCCESS)
				ret = usbg_rm_dir(path, dent[i]->d_name);

			free(dent[i]);
		}
		free(dent);
	} else {
		ret = usbg_translate_error(errno);
	}

	return ret;
}

int usbg_check_dir(const char *path)
{
	int ret = USBG_SUCCESS;
	DIR *dir;

	/* Assume that user will always have read access to this directory */
	dir = opendir(path);
	if (dir)
		closedir(dir);
	else if (errno != ENOENT || mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO) != 0)
		ret = usbg_translate_error(errno);

	return ret;
}


