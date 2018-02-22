#include <dirent.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <cmocka.h>
#include <dlfcn.h>
#include <errno.h>

typedef int (*fwrite_f_type)(const void *ptr, size_t size,
			     size_t nmemb, FILE *stream);
typedef int (*fflush_f_type)(FILE *);
typedef fflush_f_type ferror_f_type;

/**
 * @brief Simulates opening file
 * @details Checks if path is equal expected value and returns given pointer
 * from cmocka queue
 */
FILE *fopen(const char *path, const char *mode)
{
	check_expected(path);
	return mock_ptr_type(FILE *);
}

/**
 * @brief Simulates closing file
 * @details Does absolutely nothing, always acts as successfull close
 */
int fclose(FILE *fp)
{
	check_expected(fp);
	return mock_type(int);
}

/**
 * @brief Simulates reading file
 * @details Does not read any file, instead returns value from cmocka queue
 * @return value specified by caller previously
 */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	char *data;
	int len;
	size_t ret;

	check_expected(stream);
	data = mock_ptr_type(char *);
	len = mock_type(int);

	ret = size * nmemb < len ? size * nmemb : len;
	memcpy(ptr, data, ret);

	return ret;
}

/**
 * @brief Simulates opening directory
 * @details Does not open any dir, instead returns user-specified value
 * @return value specified by caller previously
 */
DIR *opendir(const char *name)
{
	int err;

	check_expected(name);
	err = mock_type(int);
	if (err)
		errno = err;

	return mock_ptr_type(DIR *);
}

/**
 * @brief Simulates closing directory
 * @details Does nothing and ends successfully.
 */
int closedir(DIR *dirp)
{
	check_expected(dirp);
	return mock_type(int);
}

/**
 * @brief Simulates scanning directory
 * @details Checks if dirp has expected value. Then consecutive values from
 * cmocka queue are proceed. First value must be integer and indicates number
 * of directory entries which should be returned. Next number of values indicate
 * names of directory entries.
 */
int scandir(const char *dirp, struct dirent ***namelist,
		int (*filter)(const struct dirent *),
		int (*compar)(const struct dirent **, const struct dirent **))
{
	int count;
	int i, j = 0;
	char *name;
	struct dirent **entries;
	struct dirent *entry;
	int tmp, expected;

	check_expected(dirp);
	count = mock_type(int);

	if (count > 0)
		entries = calloc(count, sizeof(*entries));
	else
		entries = NULL;

	for (i = 0; i < count; i++) {
		name = mock_ptr_type(char *);
		entry = malloc(sizeof(*entry));
		if (strlen(name) >= NAME_MAX)
			fail();

		strcpy(entry->d_name, name);
		entry->d_type = mock_type(unsigned char);

		expected = mock_type(int);
		if (filter) {
			tmp = filter(entry);
			assert_int_equal(tmp, expected);
			if (tmp)
				entries[j++] = entry;
			else
				free(entry);
		}
	}

	if (compar)
		qsort(entries, count, sizeof(*entries),
			(int (*)(const void *,const void *))compar);

	*namelist = entries;
	return j;
}

/**
 * @brief Simultes readlink, with user-specified behavior
 * @datails Check if path and bufsiz equal expedted values and
 * write to buf string given by cmocka
 */
ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
	char *res;
	int reslen;

	check_expected(path);
	check_expected(bufsiz);
	res = mock_ptr_type(char *);
	reslen = strlen(res);
	if (bufsiz <= reslen)
		fail();

	strcpy(buf, res);

	return reslen;
}

/**
 * @brief Simulates write, with user-specified behavior
 * @details Check if user is trying to write expected data
 * @return value received from cmocka queue
 */
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int ret;

	/*
	 * Cmocka (or anything else) may want to print some errors.
	 * Especially when running fwrite() itself
	 */
	if (stream == stderr || stream == stdout) {
		fwrite_f_type orig_fwrite;
		orig_fwrite = (fwrite_f_type)dlsym(RTLD_NEXT, "fwrite");
		return orig_fwrite(ptr, size, nmemb, stream);
	}

	check_expected(stream);
	check_expected(ptr);
	ret = mock_type(int);
	if (!ret)
		ret = nmemb;

	return ret;
}

int mkdir(const char *pathname, mode_t mode)
{
	check_expected(pathname);
	check_expected(mode);
	return mock_type(int);
}

/**
 * @brief Does nothing.
 */
int fflush(FILE *stream)
{
	if (stream == stderr || stream == stdout) {
		fflush_f_type orig_fflush;
		orig_fflush = (fflush_f_type)dlsym(RTLD_NEXT, "fflush");
		return orig_fflush(stream);
	}

	return 0;
}

int ferror(FILE *stream)
{
	if (stream == stderr || stream == stdout) {
		ferror_f_type orig_ferror;
		orig_ferror = (ferror_f_type)dlsym(RTLD_NEXT, "ferror");
		return orig_ferror(stream);
	}

	return 0;
}
