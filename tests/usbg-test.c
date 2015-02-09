#include <usbg/usbg.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <limits.h>

#include "usbg-test.h"

static struct simple_stack{
	void *ptr;
	struct simple_stack *next;
} *cleanup_top = NULL;

void free_later(void *ptr)
{
	struct simple_stack *t;

	t = malloc(sizeof(*t));
	t->ptr = ptr;
	t->next = cleanup_top;
	cleanup_top = t;
}

void cleanup_stack()
{
	struct simple_stack *t;

	while (cleanup_top) {
		free(cleanup_top->ptr);
		t = cleanup_top->next;
		free(cleanup_top);
		cleanup_top = t;
	}
}


/* Represent last file/dir opened, next should have bigger numbers.*/
static int file_id = 0;
static int dir_id = 0;

#define PUSH_FILE(file, content) do {\
	file_id++;\
	expect_path(fopen, path, file);\
	will_return(fopen, file_id);\
	expect_value(fgets, stream, file_id);\
	will_return(fgets, content);\
	expect_value(fclose, fp, file_id);\
	will_return(fclose, 0);\
} while(0)

#define PUSH_FILE_ALWAYS(dflt) do {\
	expect_any_count(fopen, path, -1);\
	will_return_always(fopen, 1);\
	expect_any_count(fgets, stream, 1);\
	will_return_always(fgets, dflt);\
	expect_any_count(fclose, fp, 1);\
	will_return_always(fclose, 0);\
} while(0)

#define PUSH_EMPTY_DIR(p) do {\
	expect_string(scandir, dirp, p);\
	will_return(scandir, 0);\
} while(0)

#define EXPECT_OPENDIR(n) do {\
	dir_id++;\
	expect_path(opendir, name, n);\
	will_return(opendir, dir_id);\
	expect_value(closedir, dirp, dir_id);\
	will_return(closedir, 0);\
} while(0)

#define PUSH_DIR(p, c) do {\
	expect_path(scandir, dirp, p);\
	will_return(scandir, c);\
} while(0)

#define PUSH_DIR_ENTRY(name, type) do {\
	will_return(scandir, name);\
	will_return(scandir, type);\
	will_return(scandir, 1);\
} while(0)

#define PUSH_LINK(p, c, len) do {\
	expect_path(readlink, path, p);\
	expect_in_range(readlink, bufsiz, len, INT_MAX);\
	will_return(readlink, c);\
} while(0)

/**
 * @brief Compare test gadgets' names
 */
static int test_gadget_cmp(struct test_gadget *a, struct test_gadget *b)
{
	return strcoll(a->name, b->name);
}

/**
 * @brief Compare test functions' names
 */
static int test_function_cmp(struct test_function *a, struct test_function *b)
{
	return strcoll(a->name, b->name);
}

/**
 * @brief Compare test configs' names
 */
static int test_config_cmp(struct test_config *a, struct test_config *b)
{
	return strcoll(a->name, b->name);
}

void prepare_config(struct test_config *c, char *path)
{
	int tmp;
	int count = 0;
	struct test_function *b;

	tmp = asprintf(&c->name, "%s.%d",
			c->label, c->id);
	if (tmp < 0)
		fail();
	free_later(c->name);

	c->path = path;

	for (b = c->bindings; b->instance; b++)
		count++;

	qsort(c->bindings, count, sizeof(*c->bindings),
		(int (*)(const void *, const void *))test_function_cmp);

}

void prepare_function(struct test_function *f, char *path)
{
	const char *func_type;
	int tmp;

	func_type = usbg_get_function_type_str(f->type);
	if (func_type == NULL)
		fail();

	tmp = asprintf(&f->name, "%s.%s",
			func_type, f->instance);
	if (tmp < 0)
		fail();
	free_later(f->name);

	f->path = path;
}

void prepare_gadget(struct test_state *state, struct test_gadget *g)
{
	struct test_config *c;
	struct test_function *f;
	char *path;
	int tmp;
	int count;

	g->path = strdup(state->path);
	if (!g->path)
		fail();

	free_later(g->path);

	tmp = asprintf(&path, "%s/%s/functions",
			g->path, g->name);
	if (tmp < 0)
		fail();
	free_later(path);

	count = 0;
	for (f = g->functions; f->instance; f++) {
		prepare_function(f, path);
		count++;
	}

	/* Path needs to be known somehow when list is empty */
	f->path = path;

	qsort(g->functions, count, sizeof(*g->functions),
		(int (*)(const void *, const void *))test_function_cmp);

	tmp = asprintf(&path, "%s/%s/configs",
			g->path, g->name);
	if (tmp < 0)
		fail();
	free_later(path);

	count = 0;
	for (c = g->configs; c->label; c++) {
		prepare_config(c, path);
		count++;
	}

	/* Path needs to be known somehow when list is empty */
	c->path = path;

	qsort(g->configs, count, sizeof(*g->configs),
		(int (*)(const void *, const void *))test_config_cmp);

}

void prepare_state(struct test_state *state)
{
	struct test_gadget *g;
	int count = 0;
	int tmp;

	tmp = asprintf(&(state->path), "%s/usb_gadget", state->configfs_path);
	if (tmp < 0)
		fail();
	free_later(state->path);


	for (g = state->gadgets; g->name; g++) {
		prepare_gadget(state, g);
		count++;
	}

	qsort(state->gadgets, count, sizeof(*state->gadgets),
		(int (*)(const void *, const void *))test_gadget_cmp);

}

/* Simulation of configfs for init */

static void push_binding(struct test_config *conf, struct test_function *binding)
{
	int tmp;
	char *s_path;
	char *d_path;

	tmp = asprintf(&s_path, "%s/%s/%s", conf->path, conf->name, binding->name);
	if (tmp < 0)
		fail();
	free_later(s_path);

	tmp = asprintf(&d_path, "%s/%s", binding->path, binding->name);
	if (tmp < 0)
		fail();
	free_later(d_path);

	PUSH_LINK(s_path, d_path, USBG_MAX_PATH_LENGTH - 1);
}

static void push_config(struct test_config *c)
{
	struct test_function *b;
	int count = 0;
	int tmp;
	char *path;

	tmp = asprintf(&path, "%s/%s", c->path, c->name);
	if (tmp < 0)
		fail();
	free_later(path);

	for (b = c->bindings; b->instance; b++)
		count++;

	PUSH_DIR(path, count);
	for (b = c->bindings; b->instance; b++) {
		PUSH_DIR_ENTRY(b->name, DT_LNK);
		push_binding(c, b);
	}
}

static void push_gadget(struct test_gadget *g)
{
	int count;
	struct test_config *c;
	struct test_function *f;
	int tmp;
	char *path;

	tmp = asprintf(&path, "%s/%s/UDC", g->path, g->name);
	if (tmp < 0)
		fail();
	free_later(path);
	PUSH_FILE(path, g->udc);

	count = 0;
	for (f = g->functions; f->instance; f++)
		count++;

	PUSH_DIR(f->path, count);
	for (f = g->functions; f->instance; f++)
		PUSH_DIR_ENTRY(f->name, DT_DIR);

	count = 0;
	for (c = g->configs; c->label; c++)
		count++;

	PUSH_DIR(c->path, count);
	for (c = g->configs; c->label; c++)
		PUSH_DIR_ENTRY(c->name, DT_DIR);

	for (c = g->configs; c->label; c++)
		push_config(c);
}

void push_init(struct test_state *state)
{
	char **udc;
	struct test_gadget *g;
	int count = 0;

	EXPECT_OPENDIR(state->path);

	for (udc = state->udcs; *udc; udc++)
		count++;

	PUSH_DIR("/sys/class/udc", count);
	for (udc = state->udcs; *udc; udc++)
		PUSH_DIR_ENTRY(*udc, DT_REG);

	count = 0;
	for (g = state->gadgets; g->name; g++)
		count++;

	PUSH_DIR(state->path, count);
	for (g = state->gadgets; g->name; g++) {
		PUSH_DIR_ENTRY(g->name, DT_DIR);
	}

	for (g = state->gadgets; g->name; g++)
		push_gadget(g);
}

void assert_func_equal(usbg_function *f, struct test_function *expected)
{
	assert_string_equal(f->instance, expected->instance);
	assert_int_equal(f->type, expected->type);
	assert_path_equal(f->path, expected->path);
}

void assert_config_equal(usbg_config *c, struct test_config *expected)
{
	int i = 0;
	usbg_binding *b;

	assert_int_equal(c->id, expected->id);
	assert_string_equal(c->label, expected->label);
	assert_path_equal(c->path, expected->path);
	usbg_for_each_binding(b, c)
		assert_func_equal(b->target, &expected->bindings[i++]);
}

void assert_gadget_equal(usbg_gadget *g, struct test_gadget *expected)
{
	usbg_config *c;
	usbg_function *f;
	int i;

	assert_string_equal(g->name, expected->name);
	assert_path_equal(g->path, expected->path);

	i = 0;
	usbg_for_each_function(f, g)
		assert_func_equal(f, &expected->functions[i++]);

	i = 0;
	usbg_for_each_config(c, g)
		assert_config_equal(c, &expected->configs[i++]);
}

void assert_state_equal(usbg_state *s, struct test_state *expected)
{
	usbg_gadget *g;
	int i = 0;

	assert_path_equal(s->path, expected->path);
	assert_path_equal(s->configfs_path, expected->configfs_path);

	usbg_for_each_gadget(g, s)
		assert_gadget_equal(g, &expected->gadgets[i++]);
}

#define SIGNUM(x) (((x) > 0) - ((x) < 0))

int path_cmp(const char *actual, const char *expected)
{
	const char *a = actual;
	const char *b = expected;

	while (*a != '\0' && *b != '\0') {
		if (*a != *b)
			break;
		do
			++a;
		while (*a == '/');

		do
			++b;
		while (*b == '/');
	}

	return SIGNUM(*a - *b);
}

int path_equal_display_error(const LargestIntegralType actual, const LargestIntegralType expected)
{
	if (path_cmp((const char *)actual, (const char *)expected) == 0) {
		return 1;
	}

	fprintf(stderr, "%s != %s\n", (const char *)actual, (const char *)expected);
	return 0;
}

void assert_path_equal(const char *actual, const char *expected)
{
	if (path_equal_display_error((const LargestIntegralType)actual, (const LargestIntegralType)expected) == 0)
		fail();
}

