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

#include <sys/queue.h>
#include <string.h>
#include <usbg/usbg.h>
#include <libconfig.h>

struct usbg_state
{
	char *path;

	TAILQ_HEAD(ghead, usbg_gadget) gadgets;
	TAILQ_HEAD(uhead, usbg_udc) udcs;
	config_t *last_failed_import;
};

struct usbg_gadget
{
	char *name;
	char *path;

	TAILQ_ENTRY(usbg_gadget) gnode;
	TAILQ_HEAD(chead, usbg_config) configs;
	TAILQ_HEAD(fhead, usbg_function) functions;
	usbg_state *parent;
	config_t *last_failed_import;
	usbg_udc *udc;
};

struct usbg_config
{
	TAILQ_ENTRY(usbg_config) cnode;
	TAILQ_HEAD(bhead, usbg_binding) bindings;
	usbg_gadget *parent;

	char *name;
	char *path;
	char *label;
	int id;
};

struct usbg_function
{
	TAILQ_ENTRY(usbg_function) fnode;
	usbg_gadget *parent;

	char *name;
	char *path;
	char *instance;
	/* Only for internal library usage */
	char *label;
	usbg_function_type type;
};

struct usbg_binding
{
	TAILQ_ENTRY(usbg_binding) bnode;
	usbg_config *parent;
	usbg_function *target;

	char *name;
	char *path;
};

struct usbg_udc
{
	TAILQ_ENTRY(usbg_udc) unode;
	usbg_state *parent;
	usbg_gadget *gadget;

	char *name;
};

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

#define ARRAY_SIZE_SENTINEL(array, size)				\
	static void __attribute__ ((unused)) array##_size_sentinel()				\
	{								\
		char array##_smaller_than_expected[			\
			(int)(ARRAY_SIZE(array) - size)]		\
			__attribute__ ((unused));			\
									\
		char array##_larger_than_expected[			\
			(int)(size - ARRAY_SIZE(array))]		\
			__attribute__ ((unused));			\
	}

#define ERROR(msg, ...) do {\
                        fprintf(stderr, "%s()  "msg" \n", \
                                __func__, ##__VA_ARGS__);\
                        fflush(stderr);\
                    } while (0)

#define ERRORNO(msg, ...) do {\
                        fprintf(stderr, "%s()  %s: "msg" \n", \
                                __func__, strerror(errno), ##__VA_ARGS__);\
                        fflush(stderr);\
                    } while (0)

/* Insert in string order */
#define INSERT_TAILQ_STRING_ORDER(HeadPtr, HeadType, NameField, ToInsert, NodeField) \
	do { \
		if (TAILQ_EMPTY((HeadPtr)) || \
			(strcmp((ToInsert)->NameField, TAILQ_FIRST((HeadPtr))->NameField) < 0)) \
			TAILQ_INSERT_HEAD((HeadPtr), (ToInsert), NodeField); \
		else if (strcmp((ToInsert)->NameField, TAILQ_LAST((HeadPtr), HeadType)->NameField) > 0) \
			TAILQ_INSERT_TAIL((HeadPtr), (ToInsert), NodeField); \
		else { \
			typeof(ToInsert) _cur; \
			TAILQ_FOREACH(_cur, (HeadPtr), NodeField) { \
				if (strcmp((ToInsert)->NameField, _cur->NameField) > 0) \
					continue; \
				TAILQ_INSERT_BEFORE(_cur, (ToInsert), NodeField); \
			} \
		} \
	} while (0)

#define STRINGS_DIR "strings"
#define CONFIGS_DIR "configs"
#define FUNCTIONS_DIR "functions"

static inline int file_select(const struct dirent *dent)
{
	if ((strcmp(dent->d_name, ".") == 0) || (strcmp(dent->d_name, "..") == 0))
		return 0;
	else
		return 1;
}

int usbg_translate_error(int error);

