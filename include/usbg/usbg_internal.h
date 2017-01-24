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

#ifndef USBG_INTERNAL_H
#define USBG_INTERNAL_H

#include <sys/queue.h>
#include <string.h>
#include <usbg/usbg.h>
#include <malloc.h>
#include <sys/types.h>
#ifdef HAS_GADGET_SCHEMES
#include "usbg_internal_libconfig.h"
#else
#include "usbg_internal_none.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file include/usbg/usbg_internal.h
 */

#ifndef offsetof
#define offsetof(type, member)  __builtin_offsetof (type, member)
#endif /* offsetof */

#ifndef container_of
#define container_of(ptr, type, field) ({                               \
                        const typeof(((type *)0)->field) *member = (ptr); \
                        (type *)( (char *)member - offsetof(type, field) ); \
                })
#endif /* container_of */

#define USBG_MAX_PATH_LENGTH PATH_MAX
/* ConfigFS just like SysFS uses page size as max size of file content */
#define USBG_MAX_FILE_SIZE 4096

struct usbg_function_type
{
	/* Name of this function type */
	char *name;

	/* OS Descriptor interface name */
	char **os_desc_iname;

	/* Called to allocate instance of function */
	int (*alloc_inst)(struct usbg_function_type *, usbg_function_type,
			  const char *, const char *, usbg_gadget *,
			  struct usbg_function **);

	/* Called to free memory used by function */
	void (*free_inst)(struct usbg_function_type *, struct usbg_function *);

	/*
	 * Called when user would like to remove this function.
	 * If this callback is provided it will be always called
	 * before rmdir on function directory. This function
	 * should check received flags and remove composed function
	 * attributes (directories) only if USBG_RM_RECURSE flag
	 * has been passed.
	 */
	int (*remove)(struct usbg_function *, int);

	/* Set the value of all given attributes */
	int (*set_attrs)(struct usbg_function *, void *);

	/* Get the value of all function attributes */
	int (*get_attrs)(struct usbg_function *, void *);

	/* Free the additional memory allocated for function attributes */
	void (*cleanup_attrs)(struct usbg_function *, void *);

	/* Should import all function attributes from libconfig format */
	int (*import)(struct usbg_function *, config_setting_t *);

	/* Should export all functions attributes to libconfig format */
	int (*export)(struct usbg_function *, config_setting_t *);
};

struct usbg_state
{
	char *path;
	char *configfs_path;

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
	struct usbg_function_type *ops;
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
				break; \
			} \
		} \
	} while (0)

#define STRINGS_DIR "strings"
#define CONFIGS_DIR "configs"
#define FUNCTIONS_DIR "functions"
#define GADGETS_DIR "usb_gadget"
#define OS_DESC_DIR "os_desc"

static inline int file_select(const struct dirent *dent)
{
	if ((strcmp(dent->d_name, ".") == 0) || (strcmp(dent->d_name, "..") == 0))
		return 0;
	else
		return 1;
}

int usbg_translate_error(int error);

char *usbg_ether_ntoa_r(const struct ether_addr *addr, char *buf);



int usbg_read_buf(const char *path, const char *name,
		  const char *file, char *buf);

int usbg_read_buf_limited(const char *path, const char *name,
			  const char *file, char *buf, int len);

int usbg_read_int(const char *path, const char *name, const char *file,
		  int base, int *dest);

#define usbg_read_dec(p, n, f, d)	usbg_read_int(p, n, f, 10, d)
#define usbg_read_hex(p, n, f, d)	usbg_read_int(p, n, f, 16, d)

int usbg_read_bool(const char *path, const char *name,
		   const char *file, bool *dest);

int usbg_read_string(const char *path, const char *name,
		     const char *file, char *buf);

int usbg_read_string_limited(const char *path, const char *name,
			     const char *file, char *buf, int len);

int usbg_read_string_alloc(const char *path, const char *name,
			   const char *file, char **dest);

int usbg_write_buf(const char *path, const char *name,
		   const char *file, const char *buf, int len);

int usbg_write_int(const char *path, const char *name, const char *file,
		   int value, const char *str);

#define usbg_write_dec(p, n, f, v)	usbg_write_int(p, n, f, v, "%d\n")
#define usbg_write_hex(p, n, f, v)	usbg_write_int(p, n, f, v, "0x%x\n")
#define usbg_write_hex16(p, n, f, v)	usbg_write_int(p, n, f, v, "0x%04x\n")
#define usbg_write_hex8(p, n, f, v)	usbg_write_int(p, n, f, v, "0x%02x\n")
#define usbg_write_bool(p, n, f, v)	usbg_write_dec(p, n, f, !!v)

int usbg_write_string(const char *path, const char *name,
		      const char *file, const char *buf);

int ubsg_rm_file(const char *path, const char *name);

int usbg_rm_dir(const char *path, const char *name);

int usbg_rm_all_dirs(const char *path);

int usbg_check_dir(const char *path);
#define usbg_config_is_int(node) (config_setting_type(node) == CONFIG_TYPE_INT)
#define usbg_config_is_string(node) \
	(config_setting_type(node) == CONFIG_TYPE_STRING)

int usbg_init_function(struct usbg_function *f,
		       struct usbg_function_type *ops,
		       usbg_function_type type,
		       const char *type_name,
		       const char *instance,
		       const char *path,
		       struct usbg_gadget *parent);

void usbg_cleanup_function(struct usbg_function *f);

#define GENERIC_ALLOC_INST(prefix, _type, _member)			\
	static int prefix##_alloc_inst(struct usbg_function_type *type, \
				       usbg_function_type type_code,	\
				       const char *instance, const char *path, \
				       struct usbg_gadget *parent,	\
				       struct usbg_function **f)	\
	{								\
		_type *ff;						\
		int ret;						\
									\
		ff = malloc(sizeof(*ff));				\
		if (!ff)						\
			return USBG_ERROR_NO_MEM;			\
									\
		ret = usbg_init_function(&ff->_member, type, type_code,	\
					 type->name, instance, path, parent); \
		if (ret != USBG_SUCCESS)				\
			goto free_func;					\
									\
		*f = &ff->_member;					\
									\
		return ret;						\
									\
	free_func:							\
		free(ff);						\
		return ret;						\
	}

#define GENERIC_FREE_INST(prefix, _type, _member)			\
	static void prefix##_free_inst(struct usbg_function_type *type,	\
				       struct usbg_function *f)		\
	{								\
		_type *ff = container_of(f, _type, _member);		\
									\
		usbg_cleanup_function(&ff->_member);			\
		free(ff);						\
	}

typedef int (*usbg_attr_get_func)(const char *, const char *, const char *, void *);
typedef int (*usbg_attr_set_func)(const char *, const char *, const char *, void *);

static inline int usbg_get_dec(const char *path, const char *name,
			   const char *attr, void *val)
{
	return usbg_read_dec(path, name, attr, (int *)val);
}

static inline int usbg_set_dec(const char *path, const char *name,
			   const char *attr, void *val)
{
	return usbg_write_dec(path, name, attr, *((int *)val));
}

static inline int usbg_get_bool(const char *path, const char *name,
			   const char *attr, void *val)
{
	return usbg_read_bool(path, name, attr, (bool *)val);
}

static inline int usbg_set_bool(const char *path, const char *name,
			   const char *attr, void *val)
{
	return usbg_write_bool(path, name, attr, *((bool *)val));
}

static inline int usbg_get_string(const char *path, const char *name,
			      const char *attr, void *val)
{
	return usbg_read_string_alloc(path, name, attr, (char **)val);
}

static inline int usbg_set_string(const char *path, const char *name,
			      const char *attr, void *val)
{
	return usbg_write_string(path, name, attr, *(char **)val);
}

int usbg_get_ether_addr(const char *path, const char *name, const char *attr,
			void *val);

int usbg_set_ether_addr(const char *path, const char *name, const char *attr,
			void *val);

int usbg_get_dev(const char *path, const char *name, const char *attr,
		 void *val);

/*
 * return:
 * 0 - if not found
 * usbg_error on error (less than 0)
 * above 0 when found suitable value
 */
typedef int (*usbg_import_node_func)(config_setting_t *root,
				     const char *node_name, void *val);

/* return 0 on success, usbg_error otherwise */
typedef int (*usbg_export_node_func)(config_setting_t *root,
				     const char *node_name, void *val);

#ifdef __cplusplus
}
#endif

#endif /* USBG_INTERNAL_H */
