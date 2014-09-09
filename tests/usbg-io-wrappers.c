#include <dirent.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <cmocka.h>

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
char *fgets(char *s, int size, FILE *stream)
{
	check_expected(stream);
	strncpy(s, mock_ptr_type(char *), size);
	return s;
}

/**
 * @brief Simulates opening directory
 * @details Does not open any dir, instead returns user-specified value
 * @return value specified by caller previously
 */
DIR *opendir(const char *name)
{
	check_expected(name);
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
 * @brief Simulates puts, with user-specified behavior
 * @details Check if user is trying to write expected data
 * @return value received from cmocka queue
 */
int fputs(const char *s, FILE *stream)
{
	/* Cmocka (or anything else) may want to print some errors.
	 * Especially when running fputs itself */
	if (stream == stderr) {
		printf("%s", s);
		return 0;
	}

	check_expected(stream);
	check_expected(s);
	return mock_type(int);
}

/**
 * @brief Does nothing.
 */
int fflush(FILE *stream)
{
	return 0;
}

int ferror(FILE *stream)
{
	return 0;
}
