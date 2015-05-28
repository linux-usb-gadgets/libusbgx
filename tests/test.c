#include <usbg/usbg.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>

#ifdef HAS_LIBCONFIG
#include <libconfig.h>
#endif

#include "usbg-test.h"

/**
 * @file tests/test.c
 */

#define USBG_TEST(name, test, setup, teardown) \
	{name, test, setup, teardown}

#define FILLED_STR(len, c) \
	{ [0 ... len - 2] = c, [len - 1] = '\0' }

/* two levels of macros allow to strigify result of macro expansion */
#define STR(s) #s
#define XSTR(s) STR(s)
/* unique string */
#define UNIQUE XSTR(__COUNTER__)

#define FUNC_FROM_TYPE(t) { \
	.type = t, \
	.instance = "instance"UNIQUE \
}

#define CONF_FROM_BOUND(b) { \
	.label = "c", \
	.id = __COUNTER__, \
	.bound_funcs = b \
}

static usbg_gadget_attrs min_gadget_attrs = {
	.bcdUSB = 0x0000,
	.bDeviceClass = 0x0,
	.bDeviceSubClass = 0x0,
	.bDeviceProtocol = 0x0,
	.bMaxPacketSize0 = 0x0,
	.idVendor = 0x0000,
	.idProduct = 0x0000,
	.bcdDevice = 0x0000
};

static usbg_gadget_attrs max_gadget_attrs = {
	.bcdUSB = 0xffff,
	.bDeviceClass = 0xff,
	.bDeviceSubClass = 0xff,
	.bDeviceProtocol = 0xff,
	.bMaxPacketSize0 = 0xff,
	.idVendor = 0xffff,
	.idProduct = 0xffff,
	.bcdDevice = 0xffff
};

/* PATH_MAX is limit for path length */
#define LONG_PATH_LEN PATH_MAX/2
static char long_path_str[] = FILLED_STR(LONG_PATH_LEN, 'x');

/* NAME_MAX is limit for filename length */
static char long_usbg_string[] = FILLED_STR(NAME_MAX, 'x');

static usbg_config_strs simple_config_strs= {
	.configuration = "configuration string"
};

static usbg_config_attrs max_config_attrs = {
	.bmAttributes = 0xff,
	.bMaxPower = 0xff
};

static usbg_config_attrs min_config_attrs = {
	.bmAttributes = 0x00,
	.bMaxPower = 0x00
};

/**
 * @brief Simplest udcs names
 * @details Used to go through init when testing other things
 */
static char *simple_udcs[] = {
	"UDC1",
	"UDC2",
	NULL
};

static char *long_udcs[] = {
	long_usbg_string,
	"UDC1",
	NULL
};

/**
 * @brief Simplest functions names
 * @details Used to go through init when testing other things
 */
static struct test_function simple_funcs[] = {
	FUNC_FROM_TYPE(F_ECM),
	FUNC_FROM_TYPE(F_ACM),
	TEST_FUNCTION_LIST_END
};

/**
 * @brief All functions types
 * @details When testing with this in state, check if all func types are
 * processed correctly
 */
static struct test_function all_funcs[] = {
	FUNC_FROM_TYPE(F_SERIAL),
	FUNC_FROM_TYPE(F_ACM),
	FUNC_FROM_TYPE(F_OBEX),
	FUNC_FROM_TYPE(F_ECM),
	FUNC_FROM_TYPE(F_SUBSET),
	FUNC_FROM_TYPE(F_NCM),
	FUNC_FROM_TYPE(F_EEM),
	FUNC_FROM_TYPE(F_RNDIS),
	FUNC_FROM_TYPE(F_PHONET),
	FUNC_FROM_TYPE(F_FFS),
	TEST_FUNCTION_LIST_END
};

static struct test_function same_type_funcs[] = {
	FUNC_FROM_TYPE(F_SERIAL),
	FUNC_FROM_TYPE(F_SERIAL),
	FUNC_FROM_TYPE(F_SERIAL),
	TEST_FUNCTION_LIST_END
};

/**
 * @brief No functions at all
 * @details Check if gadget with no functions (or config with no bindings)
 * is processed correctly.
 */
static struct test_function no_funcs[] = {
	TEST_FUNCTION_LIST_END
};

/**
 * @brief Simple configs
 * @details Used to pass through init when testing other things
 */
static struct test_config simple_confs[] = {
	CONF_FROM_BOUND(simple_funcs),
	TEST_CONFIG_LIST_END
};

/**
 * @brief Configs bound to all avaible function types
 */
static struct test_config all_bindings_confs[] = {
	CONF_FROM_BOUND(no_funcs),
	CONF_FROM_BOUND(all_funcs),
	TEST_CONFIG_LIST_END
};

#define GADGET(n, u, c, f) \
	{ \
		.name = n, \
		.udc = u, \
		.configs = c, \
		.functions = f \
	}

/**
 * @brief Simplest gadget
 */
static struct test_gadget simple_gadgets[] = {
	GADGET("g1", "UDC1", simple_confs, simple_funcs),
	TEST_GADGET_LIST_END
};

/**
 * @brief Gadgets with all avaible functions
 */
static struct test_gadget all_funcs_gadgets[] = {
	GADGET("all_funcs_gadget1", "UDC1", all_bindings_confs, all_funcs),
	TEST_GADGET_LIST_END
};

static struct test_gadget long_udc_gadgets[] = {
	GADGET("long_udc_gadgets", long_usbg_string, simple_confs, simple_funcs),
	TEST_GADGET_LIST_END
};

struct test_data {
	struct test_state *state;
	struct usbg_state *usbg_state;
};

struct test_gadget_strs_data {
	struct test_state *state;
	usbg_gadget_strs *strs;
};

#define STATE(p, g, u) { \
	.configfs_path = p, \
	.gadgets = g, \
	.udcs = u \
}

/**
 * @brief Simple state
 */
static struct test_state simple_state = STATE("config", simple_gadgets, simple_udcs);

/**
 * @brief State with all functions avaible
 */
static struct test_state all_funcs_state = STATE("all_funcs_configfs", all_funcs_gadgets, simple_udcs);

static struct test_state long_path_state = STATE(long_path_str, simple_gadgets, simple_udcs);

static struct test_state long_udc_state = STATE("simple_path", long_udc_gadgets, long_udcs);

static usbg_config_attrs *get_random_config_attrs()
{
	usbg_config_attrs *ret;

	ret = safe_malloc(sizeof(*ret));

	srand(time(NULL));
	ret->bmAttributes = rand() % max_config_attrs.bmAttributes;
	ret->bMaxPower = rand() % max_config_attrs.bMaxPower;

	return ret;
}

static usbg_gadget_attrs *get_random_gadget_attrs()
{
	usbg_gadget_attrs *ret;

	ret = safe_malloc(sizeof(*ret));

	srand(time(NULL));
	ret->bcdUSB = rand() % max_gadget_attrs.bcdUSB;
	ret->bDeviceClass = rand() % max_gadget_attrs.bDeviceClass;
	ret->bDeviceSubClass = rand() % max_gadget_attrs.bDeviceSubClass;
	ret->bDeviceProtocol = rand() % max_gadget_attrs.bDeviceProtocol;
	ret->bMaxPacketSize0 = rand() % max_gadget_attrs.bMaxPacketSize0;
	ret->idVendor = rand() % max_gadget_attrs.idVendor;
	ret->idProduct = rand() % max_gadget_attrs.idProduct;
	ret->bcdDevice = rand() % max_gadget_attrs.bcdDevice;

	return ret;
}

/**
 * @brief Add given attributes to all configs in state
 * @return Prepared state where configs has given attributes
 */
static void *prepare_state_with_config_attrs(struct test_state *state,
		usbg_config_attrs *attrs)
{
	struct test_gadget *tg;
	struct test_config *tc;

	for (tg = state->gadgets; tg->name; ++tg)
		for (tc = tg->configs; tc->label; ++tc)
			tc->attrs = attrs;

	state = prepare_state(state);
	return state;
}

static int setup_max_config_attrs_state(void **state)
{
	*state = prepare_state_with_config_attrs(&simple_state, &max_config_attrs);
	return 0;
}

static int setup_min_config_attrs_state(void **state)
{
	*state = prepare_state_with_config_attrs(&simple_state, &min_config_attrs);
	return 0;
}

static int setup_random_config_attrs_state(void **state)
{
	*state = prepare_state_with_config_attrs(&simple_state, get_random_config_attrs());
	return 0;
}

static int setup_simple_config_strs_state(void **state)
{
	struct test_gadget *tg;
	struct test_config *tc;

	for (tg = simple_state.gadgets; tg->name; ++tg)
		for (tc = tg->configs; tc->label; ++tc)
			tc->strs = &simple_config_strs;

	*state = prepare_state(&simple_state);
	return 0;
}

/**
 * @brief Prepare test_state with one gadget containing given function list
 * @details For testing only functions. We put them in a gadget as simply
 * as possible.
 * @param[in] func Pointer to list of functions
 * @return Pointer to test state with given functions
 */
static struct test_state *put_func_in_state(struct test_function *func)
{
	struct test_state *st;
	struct test_gadget *g;
	struct test_config *c;
	char **udcs;

	st = safe_calloc(1, sizeof(*st));
	/* Do not need config */
	c = safe_calloc(1, sizeof(*c));
	g = safe_calloc(2, sizeof(*g));
	udcs = safe_calloc(2, sizeof(*udcs));

	g[0].name = "g1";
	g[0].udc = "UDC1";
	g[0].configs = c;
	g[0].functions = func;
	g[0].writable = 1;

	udcs[0] = "UDC1";
	g[0].writable = 1;

	st->configfs_path = "config";
	st->gadgets = g;
	st->udcs = udcs;
	st->writable = 1;

	st = prepare_state(st);

	return st;
}

/**
 * @brief Setup simple state with some gadgets, configs and functions
 */
static int setup_simple_state(void **state)
{
	*state = prepare_state(&simple_state);
	return 0;
}

/**
 * @brief Setup state with all avaible functions
 */
static int setup_all_funcs_state(void **state)
{
	*state = prepare_state(&all_funcs_state);
	return 0;
}

/**
 * @brief Setup state with few functions of the same type
 */
static int setup_same_type_funcs_state(void **state)
{
	*state = put_func_in_state(same_type_funcs);
	return 0;
}

/**
 * @brief Setup state with very long path name
 */
static int setup_long_path_state(void **state)
{
	*state = prepare_state(&long_path_state);
	return 0;
}

/**
 * @brief Setup state with long udc name
 */
static int setup_long_udc_state(void **state)
{
	*state = prepare_state(&long_udc_state);
	return 0;
}

/**
 * @brief Setup state with gadget strings of random length
 * @param[out] state Pointer to pointer to test_gadget_strs_data structure
 * with initialized state and strings
 */
static int setup_random_len_gadget_strs_data(void **state)
{
	usbg_gadget_strs *strs;
	struct test_gadget_strs_data *data;

	/* will fill memory with zeros */
	strs = safe_calloc(1, sizeof(*strs));
	data = safe_malloc(sizeof(*data));

	srand(time(NULL));

	memset(strs->str_ser, 'x', rand() % USBG_MAX_STR_LENGTH);
	memset(strs->str_mnf, 'x', rand() % USBG_MAX_STR_LENGTH);
	memset(strs->str_prd, 'x', rand() % USBG_MAX_STR_LENGTH);

	data->strs = strs;

	data->state = prepare_state(&simple_state);
	*state = data;
	return 0;
}

/**
 * @brief Tests usbg_get_gadget function with given state
 * @details Check if gadgets are returned correctly
 */
static void test_get_gadget(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_gadget(ts, s, assert_gadget_equal);
}

/**
 * @brief Tests usbg_get_gadget with non-existing gadget name
 * @details Check if get_gadget will not find non-existing gadgets and
 * will not cause crash.
 */
static void test_get_gadget_fail(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;

	safe_init_with_state(state, &st, &s);

	g = usbg_get_gadget(s, "non-existing-gadget");
	assert_null(g);
}

/**
 * @brief Tests usbg_get_first_gadget function
 * @details Check if gadget returned by get_first_gadget is actually first one
 */
static void test_get_first_gadget(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;

	safe_init_with_state(state, &st, &s);

	g = usbg_get_first_gadget(s);
	assert_non_null(g);
	assert_gadget_equal(g, &st->gadgets[0]);
}

/**
 * @brief Tests get_first_gadget with invalid arguments
 */
static void test_get_first_gadget_fail(void **state)
{
	usbg_gadget *g;

	g = usbg_get_first_gadget(NULL);
	assert_null(g);
}

static void try_get_gadget_name(usbg_gadget *g, struct test_gadget *tg)
{
	const char *name;

	name = usbg_get_gadget_name(g);
	assert_string_equal(name, tg->name);
}

/**
 * @brief Tests getting name of gadget
 * @details Check if gadget name is returned correctly
 */
static void test_get_gadget_name(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_gadget(ts, s, try_get_gadget_name);
}

static void try_get_gadget_name_len(usbg_gadget *g, struct test_gadget *tg)
{
	int len;
	int expected;

	expected = strlen(tg->name);
	len = usbg_get_gadget_name_len(g);
	assert_int_equal(len, expected);
}

/**
 * @brief Tests getting name length of gadget
 * @details Check if returned name length is equal original
 */
static void test_get_gadget_name_len(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_gadget(ts, s, try_get_gadget_name_len);
}

/**
 * @brief Tests getting name of gadget with invalid arguments
 * @details Check if trying to get name of wrong (non-existing) gadget
 * will not cause crash, but return NULL as expected.
 */
static void test_get_gadget_name_fail(void **state)
{
	const char *name;

	name = usbg_get_gadget_name(NULL);
	assert_null(name);
}

static void try_cpy_gadget_name(usbg_gadget *g, struct test_gadget *tg)
{
	char name[USBG_MAX_NAME_LENGTH];
	int ret;

	ret = usbg_cpy_gadget_name(g, name, USBG_MAX_NAME_LENGTH);
	assert_int_equal(ret, USBG_SUCCESS);
	assert_string_equal(name, tg->name);
}

/**
 * @brief Tests copying gadget's name
 * @details Check if copying gadget name copy actual name correctly
 */
static void test_cpy_gadget_name(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_gadget(ts, s, try_cpy_gadget_name);
}

/**
 * @brief Test copying gadet name with invalid arguments
 * @details Check if trying to copy gadget name into non-existing buffer,
 * or give invalid buffer length, or invalid gadget will be handled by library
 * and return correct error codes
 */
static void test_cpy_gadget_name_fail(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;
	char name[USBG_MAX_NAME_LENGTH];
	int ret;

	safe_init_with_state(state, &st, &s);

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);

		ret = usbg_cpy_gadget_name(g, name, 0);
		assert_int_equal(ret, USBG_ERROR_INVALID_PARAM);

		ret = usbg_cpy_gadget_name(g, NULL, USBG_MAX_NAME_LENGTH);
		assert_int_equal(ret, USBG_ERROR_INVALID_PARAM);
	}

	ret = usbg_cpy_gadget_name(NULL, name, USBG_MAX_NAME_LENGTH);
	assert_int_equal(ret, USBG_ERROR_INVALID_PARAM);
}

/**
 * @brief Tests init by comparing test state and usbg state
 * @details Check if usbg state after init match given state and
 * if init returned success code
 */
static void test_init(void **state)
{
	usbg_state *s = NULL;
	struct test_state *st;

	safe_init_with_state(state, &st, &s);

	assert_state_equal(s, st);
}

/**
 * @brief Test getting function by name
 * @param[in] state Pointer to pointer to correctly initialized test_state structure
 */
static void test_get_function(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_function(ts, s, assert_func_equal);
}

/**
 * @brief Tests usbg_get_function with some non-existing functions
 * @details Check if get function will return NULL, when invalid
 * functions names and types are passed as arguments and will not cause crash.
 * @param[in] state Pointer to pointer to correctly initialized test_state structure
 */
static void test_get_function_fail(void **state)
{
	usbg_state *s = NULL;
	usbg_gadget *g = NULL;
	usbg_function *f = NULL;
	struct test_state *st;

	safe_init_with_state(state, &st, &s);

	g = usbg_get_first_gadget(s);
	assert_non_null(g);

	f = usbg_get_function(g, F_ACM, "non-existing-instance");
	assert_null(f);

	f = usbg_get_function(g, 9001, "0");
	assert_null(f);
}


/**
 * @brief Tests function type translation to string
 * @param[in] state Pointer to pointer to correctly initialized test_state structure
 * @details Check if get_function_type_str returns proper strings for all types.
 */
static void test_get_function_type_str(void **state)
{
	struct {
		usbg_function_type type;
		const char *str;
	} types[] = {
		{F_SERIAL, "gser"},
		{F_ACM, "acm"},
		{F_OBEX, "obex"},
		{F_ECM, "ecm"},
		{F_SUBSET, "geth"},
		{F_NCM, "ncm"},
		{F_EEM, "eem"},
		{F_RNDIS, "rndis"},
		{F_PHONET, "phonet"},
		{F_FFS, "ffs"},
	};

	const char *str;
	int i;

	for (i = 0; i < ARRAY_SIZE(types); i++) {
		str = usbg_get_function_type_str(types[i].type);
		assert_non_null(str);
		assert_string_equal(str, types[i].str);
	}
}

/**
 * @brief Tests function type translation to string with unknown funcs
 * @param[in] state Not used parameter
 * @details Check if get_function_type_str returns NULL, when given
 * function type is unknown.
 */
static void test_get_function_type_str_fail(void **state)
{
	const char *str;

	str = usbg_get_function_type_str(-1);
	assert_null(str);
}

/**
 * @brief Get instance of given function and check it
 * @param[in] f Usbg function
 * @param[in] tf Test function which should match f
 */
static void try_get_function_instance(usbg_function *f, struct test_function *tf)
{
	const char *str;

	str = usbg_get_function_instance(f);
	assert_string_equal(str, tf->instance);
}

/**
 * @brief Tests getting function instance from usbg_function structure
 * @param[in] state Pointer to pointer to correctly initialized test_state structure
 * @details Check if returned instance name is correct.
 */
static void test_get_function_instance(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_function(ts, s, try_get_function_instance);
}

/**
 * @brief Cpy instance of given usbg function and check it
 * @param[in] f Usbg function
 * @param[in] tf Test function which should match f
 */
static void try_cpy_function_instance(usbg_function *f, struct test_function *tf)
{
	char str[USBG_MAX_NAME_LENGTH];
	int ret;
	int small_len = 2;

	ret = usbg_cpy_function_instance(f, str, USBG_MAX_NAME_LENGTH);
	assert_int_equal(ret, USBG_SUCCESS);
	assert_string_equal(str, tf->instance);

	ret = usbg_cpy_function_instance(f, str, small_len);
	assert_int_equal(ret, USBG_SUCCESS);
	assert_memory_equal(str, tf->instance, small_len - 1);
	assert_int_equal(str[small_len - 1], '\0');
}

/**
 * @brief Tests copying function instance from usbg_function structure into buffer
 * @param[in] state Pointer to pointer to correctly initialized state
 * @details Check if buffer contains expected string
 */
static void test_cpy_function_instance(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_function(ts, s, try_cpy_function_instance);
}

/**
 * @brief Get function type and check it
 * @param[in] f Usbg function
 * @param[in] tf Test function which should match f by type
 */
static void try_get_function_type(usbg_function *f, struct test_function *tf)
{
	usbg_function_type type;

	type = usbg_get_function_type(f);
	assert_int_equal(type, tf->type);
}

/**
 * @brief Tests getting function type
 * @details Check if getting function type returns what was expected.
 * State must be proper (init must end with success).
 * @param[in] state Pointer to pointer to correctly initialized state
 */
static void test_get_function_type(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_function(ts, s, try_get_function_type);
}

/**
 * @brief Check if function instance length is correct
 * @param[in] f Usbg function
 * @param[in] tf Test function which should match f
 */
static void try_get_function_instance_len(usbg_function *f, struct test_function *tf)
{
	size_t len;
	len = usbg_get_function_instance_len(f);
	assert_int_equal(len, strlen(tf->instance));
}

/**
 * @brief Tests getting length of function instance name
 * @details Check if returned instance name length matches
 * actual length of instance name
 * @param[in] state Pointer to pointer to correctly initialized state
 */
static void test_get_function_instance_len(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_function(ts, s, try_get_function_instance_len);
}

/**
 * @brief Tests getting configfs path from usbg state
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_configfs_path(void **state)
{
	usbg_state *s = NULL;
	struct test_state *st;
	const char *path;

	safe_init_with_state(state, &st, &s);

	path = usbg_get_configfs_path(s);
	assert_path_equal(path, st->configfs_path);
}

/**
 * @brief Tests getting configfs path length from usbg state
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_configfs_path_len(void **state)
{
	usbg_state *s = NULL;
	struct test_state *st;
	int ret, len;

	safe_init_with_state(state, &st, &s);

	ret = usbg_get_configfs_path_len(s);
	len = strlen(st->configfs_path);
	assert_int_equal(ret, len);
}

/**
 * @brief Tests copying configfs path into buffer
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_cpy_configfs_path(void **state)
{
	usbg_state *s = NULL;
	struct test_state *st;
	char path[PATH_MAX];
	int ret;

	safe_init_with_state(state, &st, &s);

	ret = usbg_cpy_configfs_path(s, path, PATH_MAX);
	assert_int_equal(ret, USBG_SUCCESS);
	assert_path_equal(path, st->configfs_path);
}

/**
 * @brief Tests getting config by name
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_config(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_config(ts, s, assert_config_equal);
}

static void test_get_config_without_label(void **state)
{
	usbg_state *s = NULL;
	usbg_gadget *g = NULL;
	usbg_config *c = NULL;
	struct test_state *ts;
	struct test_gadget *tg;
	struct test_config *tc;

	safe_init_with_state(state, &ts, &s);

	for (tg = ts->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);
		assert_non_null(g);
		for (tc = tg->configs; tc->label; tc++) {
			c = usbg_get_config(g, tc->id, NULL);
			assert_non_null(c);
			assert_config_equal(c, tc);
		}
	}
}

/**
 * @bried Tests getting non-existing config
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_config_fail(void **state)
{
	usbg_state *s = NULL;
	usbg_gadget *g = NULL;
	usbg_config *c = NULL;
	struct test_state *ts;
	struct test_gadget *tg;

	safe_init_with_state(state, &ts, &s);

	for (tg = ts->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);
		assert_non_null(g);

		c = usbg_get_config(g, 0, "non-existing-config");
		assert_null(c);

		c = usbg_get_config(g, -9001, "c");
		assert_null(c);

		c = usbg_get_config(g, -9001, NULL);
		assert_null(c);
	}
}

/**
 * @brief Get config label and check it
 * @param[in] c Usbg config
 * @param[in] tc Test config which should match c
 */
static void try_get_config_label(usbg_config *c, struct test_config *tc)
{
	const char *label;
	label = usbg_get_config_label(c);
	assert_string_equal(label, tc->label);
}

/**
 * @brief Tests getting config label
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_config_label(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_config(ts, s, try_get_config_label);
}

/**
 * @brief Check config id with test structure
 * @param[in] c Usbg config
 * @param[in] tc Test config which should match c
 */
static void try_get_config_id(usbg_config *c, struct test_config *tc)
{
	int id;
	id = usbg_get_config_id(c);
	assert_int_equal(id, tc->id);
}

/**
 * @brief Tests getting config id
 * @param[in,out] state Pointer to pointer to correctly initialized test state.
 * When finished, it contains pointer to usbg_state which should be cleaned.
 */
static void test_get_config_id(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_config(ts, s, try_get_config_id);
}

/**
 * @brief Test getting given attributes from gadgets present in state
 * @param[in] s Pointer to usbg state
 * @param[in] ts Pointer to test state matching given usbg state
 * @param[in] attrs Pointer to gadget attributes which should be put in
 * virtual filesystem for writting by usbg
 */
static void try_get_gadget_attrs(usbg_state *s, struct test_state *ts,
		usbg_gadget_attrs *attrs)
{
	usbg_gadget *g = NULL;
	usbg_gadget_attrs actual;
	struct test_gadget *tg;
	int ret;

	for (tg = ts->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);
		assert_non_null(g);

		push_gadget_attrs(tg, attrs);
		ret = usbg_get_gadget_attrs(g, &actual);

		assert_int_equal(ret, 0);
		assert_gadget_attrs_equal(&actual, attrs);
	}
}

/**
 * @brief Tests getting gadget attributes
 * @param[in] state Pointer to correctly initialized test_state structure
 **/
static void test_get_gadget_attrs(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);

	try_get_gadget_attrs(s, ts, &min_gadget_attrs);
	try_get_gadget_attrs(s, ts, &max_gadget_attrs);
	try_get_gadget_attrs(s, ts, get_random_gadget_attrs());
}

/**
 * @brief Test setting given attributes on gadgets present in state
 * @param[in] s Pointer to usbg state
 * @param[in] ts Pointer to test state matching given usbg state
 * @param[in] attrs Pointer to gadget attributes to be set
 */
static void try_set_gadget_attrs(usbg_state *s, struct test_state *ts,
		usbg_gadget_attrs *attrs)
{
	usbg_gadget *g = NULL;
	struct test_gadget *tg;
	int ret;

	for (tg = ts->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);
		assert_non_null(g);

		pull_gadget_attrs(tg, attrs);
		ret = usbg_set_gadget_attrs(g, attrs);

		assert_int_equal(ret, 0);
	}
}
/**
 * @brief Tests setting gadget attributes
 * @param[in] state Pointer to correctly initialized test_state structure
 **/
static void test_set_gadget_attrs(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);

	try_set_gadget_attrs(s, ts, &min_gadget_attrs);
	try_set_gadget_attrs(s, ts, &max_gadget_attrs);
	try_set_gadget_attrs(s, ts, get_random_gadget_attrs());
}

/**
 * @brief Test setting given attributes on gadgets present in state one by one,
 * using functions specific for each attribute
 * @param[in] s Pointer to usbg state
 * @param[in] ts Pointer to test state matching given usbg state
 * @param[in] attrs Pointer to gadget attributes to be set
 */
static void try_set_specific_gadget_attr(usbg_state *s, struct test_state *ts,
		usbg_gadget_attrs *attrs)
{
	usbg_gadget *g = NULL;
	struct test_gadget *tg;
	int ret;
	int i;
	int attr;

	for (tg = ts->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);
		assert_non_null(g);

		for (i = USBG_GADGET_ATTR_MIN; i < USBG_GADGET_ATTR_MAX; i++) {
			attr = get_gadget_attr(attrs, i);
			pull_gadget_attribute(tg, i, attr);
			usbg_set_gadget_attr(g, i, attr);
			assert_int_equal(ret, 0);
		}
	}
}

/**
 * @brief Tests setting gadget attributes one by one
 * @param[in] state Pointer to correctly initialized test_state structure
 **/
static void test_set_specific_gadget_attr(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);

	try_set_specific_gadget_attr(s, ts, &min_gadget_attrs);
	try_set_specific_gadget_attr(s, ts, &max_gadget_attrs);
	try_set_specific_gadget_attr(s, ts, get_random_gadget_attrs());
}

/**
 * @brief Tests getting udc from state
 * @param[in] state Pointer to correctly initialized test_state structure
 **/
void test_get_udc(void **state)
{
	struct test_state *ts;
	char **tu;
	struct test_gadget *tg;
	usbg_state *s = NULL;
	usbg_udc *u = NULL;
	usbg_gadget *g = NULL;

	safe_init_with_state(state, &ts, &s);

	for (tu = ts->udcs; *tu; tu++) {
		u = usbg_get_udc(s, *tu);
		assert_non_null(u);
		assert_string_equal(*tu, u->name);
		assert_int_equal(s, u->parent);
	}

	for (tg = ts->gadgets; tg->name; tg++) {
		u = usbg_get_udc(s, tg->udc);
		g = usbg_get_gadget(s, tg->name);
		assert_int_equal(u->gadget, g);
	}
}

static void test_get_gadget_attr_str(void **state)
{
	struct {
		usbg_gadget_attr attr;
		const char *str;
	} attrs[] = {
		{BCD_USB, "bcdUSB"},
		{B_DEVICE_CLASS, "bDeviceClass"},
		{B_DEVICE_SUB_CLASS, "bDeviceSubClass"},
		{B_DEVICE_PROTOCOL, "bDeviceProtocol"},
		{B_MAX_PACKET_SIZE_0, "bMaxPacketSize0"},
		{ID_VENDOR, "idVendor"},
		{ID_PRODUCT, "idProduct"},
		{BCD_DEVICE, "bcdDevice"},
	};

	const char *str;
	int i, j;

	for (i = 0; i < ARRAY_SIZE(attrs); i++) {
		str = usbg_get_gadget_attr_str(attrs[i].attr);
		assert_non_null(str);
		assert_string_equal(str, attrs[i].str);
	}

	/* Check if iteration over values works */
	for (i = USBG_GADGET_ATTR_MIN; i < USBG_GADGET_ATTR_MAX; ++i) {
		str = usbg_get_gadget_attr_str(i);
		assert_non_null(str);

		for (j = 0; j < ARRAY_SIZE(attrs); ++j)
			if (attrs[j].attr == i) {
				assert_string_equal(str, attrs[j].str);
				break;
			}

		assert_int_not_equal(j, ARRAY_SIZE(attrs));
	}
}

static void test_get_gadget_attr_str_fail(void **state)
{
	const char *str;

	str = usbg_get_gadget_attr_str(USBG_GADGET_ATTR_MIN - 1);
	assert_null(str);

	str = usbg_get_gadget_attr_str(USBG_GADGET_ATTR_MAX);
	assert_null(str);
}

/**
 * @brief set gadget strings
 * @details Also do it one by one
 * @param[in] data Pointer to correctly initialized test_gadget_strs_data structure
 */
static void test_set_gadget_strs(void **data)
{
	struct test_gadget_strs_data *ts;
	struct test_gadget *tg;
	usbg_state *s = NULL;
	usbg_gadget *g = NULL;
	int i;
	int ret;

	ts = (struct test_gadget_strs_data *)(*data);
	*data = NULL;

	init_with_state(ts->state, &s);
	*data = s;

	for (tg = ts->state->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);

		pull_gadget_strs(tg, LANG_US_ENG, ts->strs);
		ret = usbg_set_gadget_strs(g, LANG_US_ENG, ts->strs);
		assert_int_equal(ret, 0);

		for (i = 0; i < GADGET_STR_MAX; i++)
			pull_gadget_string(tg, LANG_US_ENG, i, get_gadget_str(ts->strs, i));

		ret = usbg_set_gadget_serial_number(g, LANG_US_ENG, ts->strs->str_ser);
		assert_int_equal(ret, 0);

		ret = usbg_set_gadget_manufacturer(g, LANG_US_ENG, ts->strs->str_mnf);
		assert_int_equal(ret, 0);

		ret = usbg_set_gadget_product(g, LANG_US_ENG, ts->strs->str_prd);
		assert_int_equal(ret, 0);
	}
}

/**
 * @brief get gadget strings
 * @param[in] data Pointer to correctly initialized test_gadget_strs_data structure
 */
static void test_get_gadget_strs(void **data)
{
	struct test_gadget_strs_data *ts;
	struct test_gadget *tg;
	usbg_state *s = NULL;
	usbg_gadget *g = NULL;
	usbg_gadget_strs strs;

	ts = (struct test_gadget_strs_data *)(*data);
	*data = NULL;

	init_with_state(ts->state, &s);
	*data = s;

	for (tg = ts->state->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);
		push_gadget_strs(tg, LANG_US_ENG, ts->strs);
		usbg_get_gadget_strs(g, LANG_US_ENG, &strs);
		assert_gadget_strs_equal(&strs, ts->strs);
	}
}

/**
 * @brief Get binding target
 * @details Check if given function is target of given binding
 * @param[in] tb Test function
 * @param[in] b Binding
 */
static void try_get_binding_target(struct test_binding *tb, usbg_binding *b)
{
	usbg_function *f;

	f = usbg_get_binding_target(b);
	assert_non_null(f);
	assert_func_equal(f, tb->target);
}

/**
 * @brief Test get binding target
 * @details Test all bindings present in given state
 * @param[in, out] state Pointer to pointer to correctly initialized test state,
 * will point to usbg state when finished.
 */
static void test_get_binding_target(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_binding(ts, s, try_get_binding_target);
}

/**
 * @brief Get binding name
 * @details Check if name of given binding is equal name of given function
 * @param[in] tb Test function
 * @param[in] b Binding
 */
static void try_get_binding_name(struct test_binding *tb, usbg_binding *b)
{
	const char *s;

	s = usbg_get_binding_name(b);
	assert_non_null(s);
	assert_string_equal(s, tb->name);
}

/**
 * @brief Test get bindig name from all binding present in state
 * @param[in, out] state Pointer to pointer to correctly initialized test state,
 * will point to usbg state when finished.
 */
static void test_get_binding_name(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_binding(ts, s, try_get_binding_name);
}

/**
 * @brief Get binding name length
 * @param[in] tb Test function
 * @param[in] b Binding
 */
static void try_get_binding_name_len(struct test_binding *tb, usbg_binding *b)
{
	int n;

	n = usbg_get_binding_name_len(b);
	assert_int_equal(n, strlen(tb->name));
}

/**
 * @brief Test get binding name length from all bindings present in state
 * @param[in, out] state Pointer to pointer to correctly initialized test state,
 * will point to usbg state when finished.
 */
static void test_get_binding_name_len(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_binding(ts, s, try_get_binding_name_len);
}

/**
 * @brief Set config strings
 * @param[in] c Config on which string should be set
 * @param[in] tc Test config containing strings to be set
 */
static void try_set_config_strs(usbg_config *c, struct test_config *tc)
{
	pull_config_strs(tc, LANG_US_ENG, tc->strs);
	usbg_set_config_strs(c, LANG_US_ENG, tc->strs);
}

/**
 * @brief Test setting strings
 * @details Set strings in all configs present in state
 * @param[in, out] state Pointer to pointer to correctly initialized test state,
 * will point to usbg state when finished.
 */
static void test_set_config_strs(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_config(ts, s, try_set_config_strs);
}

/**
 * @brief Set strings one by one on config
 * @param[in] c Config on which string should be set
 * @param[in] tc Test config containing strings to be set
 */
static void try_set_config_string(usbg_config *c, struct test_config *tc)
{
	pull_config_string(tc, LANG_US_ENG, tc->strs->configuration);
	usbg_set_config_string(c, LANG_US_ENG, tc->strs->configuration);
}

/**
 * @brief Test setting strings one by one
 * @details Set strings on all configs present in given state
 * @param[in, out] state Pointer to pointer to correctly initialized test state,
 * will point to usbg state when finished.
 */
static void test_set_config_string(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_config(ts, s, try_set_config_string);
}

/**
 * @brief Get config strings
 * @details Assume that given configs have the same strings
 * @param[in] c Config from which strings should be get
 * @param[in] tc Test config expected to have the same string as c
 */
static void try_get_config_strs(usbg_config *c, struct test_config *tc)
{
	usbg_config_strs strs;
	push_config_strs(tc, LANG_US_ENG, tc->strs);
	usbg_get_config_strs(c, LANG_US_ENG, &strs);
	assert_string_equal(tc->strs->configuration, strs.configuration);
}

/**
 * @brief Test getting congig strings
 * @details Get config strings on all configs present in given state
 * @param[in, out] state Pointer to pointer to correctly initialized test state,
 * will point to usbg state when finished.
 */
static void test_get_config_strs(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_config(ts, s, try_get_config_strs);
}

/**
 * @brief Get config attributes
 * @details Assume that attributes which will be returned are the same as
 * given test state contains.
 * @param[in] c Usbg config
 * @param[in] tc Test config with set attributes
 */
static void try_get_config_attrs(usbg_config *c, struct test_config *tc)
{
	usbg_config_attrs attrs;

	push_config_attrs(tc, tc->attrs);
	usbg_get_config_attrs(c, &attrs);
	assert_config_attrs_equal(tc->attrs, &attrs);
}

/**
 * @brief Test getting config attributes
 * @details Get config attributes on all configfs in state
 * @param[in, out] state Pointer to pointer to correctly initialized test state,
 * will point to usbg state when finished.
 */
static void test_get_config_attrs(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_config(ts, s, try_get_config_attrs);
}

/**
 * @brief Set config attributes in given config
 * @param[in] c Usbg config
 * @param[in] tc Test config with attributes which will be set
 */
static void try_set_config_attrs(usbg_config *c, struct test_config *tc)
{
	pull_config_attrs(tc, tc->attrs);
	usbg_set_config_attrs(c, tc->attrs);
}

/**
 * @brief Test setting config attributes
 * @details Set config attributes on all configs in state
 * @param[in, out] state Pointer to pointer to correctly initialized test state,
 * will point to usbg state when finished.
 */
static void test_set_config_attrs(void **state)
{
	usbg_state *s = NULL;
	struct test_state *ts;

	safe_init_with_state(state, &ts, &s);
	for_each_test_config(ts, s, try_set_config_attrs);
}

/**
 * @brieg Test creating config
 * @details Start with empty gadgets, add all functions from given state
 * @param[in, out] state Pointer to pointer to correctly initialized test state,
 * will point to usbg state when finished.
 */
static void test_create_config(void **state)
{
	usbg_state *s = NULL;
	usbg_gadget *g = NULL;
	usbg_config *c = NULL;
	struct test_state *ts;
	struct test_state *empty;
	struct test_gadget *tg;
	struct test_config *tc;

	ts = (struct test_state *)(*state);
	*state = NULL;

	empty = build_empty_gadget_state(ts);

	init_with_state(empty, &s);
	*state = s;

	for (tg = ts->gadgets; tg->name; tg++) {
		g = usbg_get_gadget(s, tg->name);
		assert_non_null(g);
		for (tc = tg->configs; tc->label; tc++) {
			pull_create_config(tc);
			usbg_create_config(g, tc->id, tc->label,
					tc->attrs, tc->strs, &c);
			assert_config_equal(c, tc);
		}
	}
}

/**
 *
 * @brief cleanup usbg state
 */
static int teardown_state(void **state)
{
	usbg_state *s = NULL;

	s = (usbg_state *)(*state);
	if (s != NULL)
		usbg_cleanup(s);

	cleanup_stack();
	return 0;
}

/* Custom macro for defining test with given name and fixed teardown function */
#define USBG_TEST_TS(name, test, setup) \
	USBG_TEST(name, test, setup, teardown_state)

/**
 * @page usbg_tests Tests
 * @brief This is list of test cases
 * @tests_start
 */

#ifndef DOXYGEN
static struct CMUnitTest tests[] = {
#endif

	/**
	 * @usbg_test
	 * @test_desc{test_init_simple,
	 * Check if init was successfull on simple configfs state,
	 * usbg_init}
	 */
	USBG_TEST_TS("test_init_simple",
		     test_init, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_init_all_funcs,
	 * Check if init was successfull when all avaible functions
	 * are present in configfs,
	 * usbg_init}
	 */
	USBG_TEST_TS("test_init_all_funcs",
		     test_init, setup_all_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_init_long_path,
	 * Try to initialize libusbg with long configfs path,
	 * usbg_init}
	 */
	USBG_TEST_TS("test_init_long_path",
		     test_init, setup_long_path_state),
	/**
	 * @usbg_test
	 * @test_desc{test_init_long_udc,
	 * Try to initialize libusbg with long udc name,
	 * usbg_init}
	 */
	USBG_TEST_TS("test_init_long_udc",
		     test_init, setup_long_udc_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_simple,
	 * Check if simple gadget will be correcty returned,
	 * usbg_get_gadget}
	 */
	USBG_TEST_TS("test_get_gadget_simple",
		     test_get_gadget, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_fail_simple,
	 * Check if getting non-existing and wrong gadgets cause
	 * expected failure and error codes are correct,
	 * usbg_get_gadget}
	 */
	USBG_TEST_TS("test_get_gadget_fail_simple",
		     test_get_gadget_fail, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_first_gadget_simple,
	 * Check if gadget returned by get_first_gadget
	 * is actually first one,
	 * usbg_get_first_gadget}
	 */
	USBG_TEST_TS("test_get_first_gadget_simple",
		     test_get_first_gadget, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_first_gadget_fail,
	 * Check if getting first gadget from state returns NULL when
	 * invalid parameters are passed,
	 * usbg_get_first_gadget}
	 */
	unit_test(test_get_first_gadget_fail),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_name_simple,
	 * Check if returned gadget name matches expected value,
	 * usbg_get_gadget_name}
	 */
	USBG_TEST_TS("test_get_gadget_name_simple",
		     test_get_gadget_name, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_name_len,
	 * Check if returned simple gadget name length matches expected value,
	 * usbg_get_gadget_name}
	 */
	USBG_TEST_TS("test_get_gadget_name_len_simple",
		     test_get_gadget_name_len, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_name_fail,
	 * Check if trying to get name of invalid gadget
	 * cause expected failure (name is null),
	 * usbg_get_gadget_name}
	 */
	unit_test(test_get_gadget_name_fail),
	/**
	 * @usbg_test
	 * @test_desc{test_cpy_gadget_name_simple,
	 * Check if getting simple gadget name into buffer work as expected,
	 * usbg_cpy_gadget_name}
	 */
	USBG_TEST_TS("test_cpy_gadget_name_simple",
		     test_cpy_gadget_name, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_cpy_gadget_name_fail_simple,
	 * Check if writting gadget name into buffer fail when
	 * invalid parameters are passed,
	 * usbg_cpy_gadget_name}
	 */
	USBG_TEST_TS("test_cpy_gadget_name_fail_simple",
		     test_cpy_gadget_name_fail, setup_simple_state),
	/**
	* @usbg_test
	 * @test_desc{test_get_function_simple,
	 * Check if function can be correctly get from simple state,
	 * usbg_get_function}
	 */
	USBG_TEST_TS("test_get_function_simple",
		     test_get_function, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_all_funcs,
	 * Check if getting function work on all function types,
	 * usbg_get_function}
	 */
	USBG_TEST_TS("test_get_function_all_funcs",
		     test_get_function, setup_all_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_same_type_funcs,
	 * Check if having multiple functions with the same type does not
	 * cause failure
	 * usbg_get_function}
	 */
	USBG_TEST_TS("test_get_function_same_type_funcs",
		     test_get_function, setup_same_type_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_fail_simple,
	 * Check if trying to get invalid function's name ends
	 * with expected error,
	 * usbg_get_function}
	 */
	USBG_TEST_TS("test_get_function_fail_simple",
		     test_get_function_fail, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_instance_simple,
	 * Check if getting simple instance returns what expected,
	 * usbg_get_function_instance}
	 */
	USBG_TEST_TS("test_get_function_instance_simple",
		     test_get_function_instance, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_cpy_function_instance_simple,
	 * Check if copying simple instance into buffer returns what expected,
	 * usbg_cpy_function_instance}
	 */
	USBG_TEST_TS("test_cpy_function_instance_simple",
		     test_cpy_function_instance, setup_all_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_type_simple,
	 * Check if function type is returned correctly,
	 * usbg_get_function_type}
	 */
	USBG_TEST_TS("test_get_function_type_simple",
		     test_get_function_type, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_type_all_funcs,
	 * Check if all function types are returned correctly,
	 * usbg_get_function_type}
	 */
	USBG_TEST_TS("test_get_function_type_all_funcs",
		     test_get_function_type, setup_all_funcs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_instance_len_simple,
	 * Check if function instance length is returned correctly,
	 * usbg_get_function_instance_len}
	 */
	USBG_TEST_TS("test_get_function_instance_len_simple",
		     test_get_function_instance_len, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_type_str,
	 * Compare returned function types strings with expected values,
	 * usbg_get_function_type_str}
	 */
	unit_test(test_get_function_type_str),
	/**
	 * @usbg_test
	 * @test_desc{test_get_function_type_str_fail,
	 * Try to get type string of unknown type,
	 * usbg_get_function_type_str}
	 */
	unit_test(test_get_function_type_str_fail),
	/**
	 * @usbg_test
	 * @test_desc{test_get_configfs_path_simple,
	 * heck if simple configfs path was returned correctly,
	 * usbg_get_configfs_path}
	 */
	USBG_TEST_TS("test_get_configfs_path_simple",
		     test_get_configfs_path, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_configfs_path_len,
	 * Check if configfs path length is correctly calculated,
	 * usbg_get_configfs_path_len}
	 */
	USBG_TEST_TS("test_get_configfs_path_len_simple",
		     test_get_configfs_path_len, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_cpy_configfs_path_simple,
	 * Copy simple configfs path into buffer and compare with original,
	 * usbg_cpy_configfs_path}
	 */
	USBG_TEST_TS("test_cpy_configfs_path_simple",
		     test_cpy_configfs_path, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_simple,
	 * Check if returned simple config matches original one,
	 * usbg_get_config}
	 */
	USBG_TEST_TS("test_get_config_simple",
		     test_get_config, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_without_label_simple,
	 * Check if returned simple config matches original one,
	 * usbg_get_config}
	 */
	USBG_TEST_TS("test_get_config_without_label_simple",
		     test_get_config_without_label, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_fail,
	 * Check if trying to get non-existing or invalid config
	 * fails as expected,
	 * usbg_get_config}*/
	USBG_TEST_TS("test_get_config_fail",
		     test_get_config_fail, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_label_simple,
	 * Check if returned simple config label matches original one,
	 * usbg_get_config_label}
	 */
	USBG_TEST_TS("test_get_config_label_simple",
		     test_get_config_label, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_id_simple,
	 * Check if returned simple config id matches original one,
	 * usbg_get_config_id}
	 */
	USBG_TEST_TS("test_get_config_id_simple",
		     test_get_config_id, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_attrs_simple,
	 * Get gadget attributes list and compare them with original,
	 * usbg_get_gadget_attrs}
	 */
	USBG_TEST_TS("test_get_gadget_attrs_simple",
		     test_get_gadget_attrs, setup_simple_state),
	/**
	 * @usbg_tets
	 * @test_desc{test_set_gadget_attrs_simple,
	 * Set gadget attributes list\, check if everything is wrote
	 * as expected,
	 * usbg_set_gadget_attrs}
	 */
	USBG_TEST_TS("test_set_gadget_attrs_simple",
		     test_set_gadget_attrs, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_set_specific_gadget_attr_simple,
	 * Set gadget attributes one by one,
	 * usbg_set_gadget_attrs}
	 */
	USBG_TEST_TS("test_set_specific_gadget_attr_simple",
		     test_set_specific_gadget_attr, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_udc_simple,
	 * Get udc name from state,
	 * usbg_get_udc}
	 */
	USBG_TEST_TS("test_get_udc_simple",
		     test_get_udc, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_udc_long,
	 * Get udc name witch very long name,
	 * usbg_get_udc}
	 */
	USBG_TEST_TS("test_get_udc_long",
		     test_get_udc, setup_long_udc_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_attr_str,
	 * Compare returned gadget attribute strings witch expected values
	 * usbg_get_gadget_attr_str}
	 */
	unit_test(test_get_gadget_attr_str),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_attr_str_fail,
	 * Check returned gadget attribute strings for invalid arguments
	 * usbg_get_gadget_attr_str}
	 */
	unit_test(test_get_gadget_attr_str_fail),
	/**
	 * @usbg_test
	 * @test_desc{test_set_gadget_strs_random,
	 * Set gadget strings of random length,
	 * usbg_set_gadget_strs}
	 */
	USBG_TEST_TS("test_set_gadget_strs_random",
		     test_set_gadget_strs, setup_random_len_gadget_strs_data),
	/**
	 * @usbg_test
	 * @test_desc{test_get_gadget_strs_random,
	 * Get gadget strings,
	 * usbg_get_gadget_strs}
	 */
	USBG_TEST_TS("test_get_gadget_strs_random",
		     test_get_gadget_strs, setup_random_len_gadget_strs_data),
	/**
	 * @usbg_test
	 * @test_desc{test_get_binding_target_simple,
	 * Get binding target,
	 * usbg_get_binding_target}
	 */
	USBG_TEST_TS("test_get_binding_target_simple",
		     test_get_binding_target, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_binding_name_simple,
	 * Get binding name,
	 * usbg_get_binding_name}
	 */
	USBG_TEST_TS("test_get_binding_name_simple",
		     test_get_binding_name, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_binding_name_len_simple,
	 * Get binding name length,
	 * usbg_get_binding_name_len}
	 */
	USBG_TEST_TS("test_get_binding_name_len_simple",
		     test_get_binding_name_len, setup_simple_state),
	/**
	 * @usbg_test
	 * @test_desc{test_set_config_strs_simple,
	 * Set simple strings in set of configurations,
	 * usbg_set_config_strs}
	 */
	USBG_TEST_TS("test_set_config_strs_simple",
		     test_set_config_strs, setup_simple_config_strs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_set_config_string_simple,
	 * Set simple string in set of configurations,
	 * usbg_set_config_string}
	 */
	USBG_TEST_TS("test_set_config_string_simple",
		     test_set_config_string, setup_simple_config_strs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_strs_simple,
	 * Get simple strings from set of configurations,
	 * usbg_get_config_strs}
	 */
	USBG_TEST_TS("test_get_config_strs_simple",
		     test_get_config_strs, setup_simple_config_strs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_attrs_max,
	 * Get config attributes with max values,
	 * usbg_get_config_attrs}
	 */
	USBG_TEST_TS("test_get_config_attrs_max",
		     test_get_config_attrs, setup_max_config_attrs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_attrs_min,
	 * Get config attributes with minimum values,
	 * usbg_get_config_attrs}
	 */
	USBG_TEST_TS("test_get_config_attrs_min",
		     test_get_config_attrs, setup_min_config_attrs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_get_config_attrs_random,
	 * Get config attributes with random values,
	 * usbg_get_config_attrs}
	 */
	USBG_TEST_TS("test_get_config_attrs_random",
		     test_get_config_attrs, setup_random_config_attrs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_set_config_attrs_max,
	 * Set config attributes with max values,
	 * usbg_set_config_attrs}
	 */
	USBG_TEST_TS("test_set_config_attrs_max",
		     test_set_config_attrs, setup_max_config_attrs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_set_config_attrs_min,
	 * Set config attributes with minimum values,
	 * usbg_set_config_attrs}
	 */
	USBG_TEST_TS("test_set_config_attrs_min",
		     test_set_config_attrs, setup_min_config_attrs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_set_config_attrs_random,
	 * Set config attributes with random values,
	 * usbg_set_config_attrs}
	 */
	USBG_TEST_TS("test_set_config_attrs_random",
		     test_set_config_attrs, setup_random_config_attrs_state),
	/**
	 * @usbg_test
	 * @test_desc{test_create_config_random,
	 * Create config with random attributes
	 * usbg_create_config}
	 */
	USBG_TEST_TS("test_create_config_random",
		     test_create_config, setup_random_config_attrs_state),

#ifndef DOXYGEN
};
#endif

/**
 * @usbg_test
 * @tests_end
 */

#define TESTS_TAG "tests"
/* for autotools compability */
#define SKIPPED_CODE 77

#ifdef HAS_LIBCONFIG

static int gen_test_config(FILE *output)
{
	config_t cfg;
	config_setting_t *root;
	config_setting_t *tests_node, *node;
	int i;
	int ret = SKIPPED_CODE, cfg_ret = 0;

	config_init(&cfg);
	config_set_tab_width(&cfg, 4);

	root = config_root_setting(&cfg);
	tests_node = config_setting_add(root, TESTS_TAG, CONFIG_TYPE_LIST);
	if (!tests_node) {
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		node = config_setting_add(tests_node, NULL, CONFIG_TYPE_STRING);
		if (!node) {
			ret = -ENOMEM;
			goto out;
		}

		cfg_ret = config_setting_set_string(node, tests[i].name);
		if (cfg_ret != CONFIG_TRUE) {
			ret = -EINVAL;
			goto out;
		}
	}

	config_write(&cfg, output);
out:
	config_destroy(&cfg);
	return ret;
}

#else

static int gen_test_config(FILE *output)
{
	fprintf(stderr, "Libconfig is not supported\n");
	return -ENOTSUP;
}

#endif /* HAS_LIBCONFIG */

static int lookup_test(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(tests); ++i)
		if (!strcmp(name, tests[i].name))
			return i;

	return -1;
}

static void test_skipped(void **state)
{
	skip();
}

#ifdef HAS_LIBCONFIG
static int apply_test_config(FILE *input)
{
	config_t cfg;
	config_setting_t *root;
	config_setting_t *tests_node, *node;
	int i, count, ind;
	int ret = 0, cfg_ret = 0;
	const char *test_name;
	char selected[ARRAY_SIZE(tests)];

	for (i = 0; i < ARRAY_SIZE(selected); ++i)
		selected[i] = 0;

	config_init(&cfg);

	cfg_ret = config_read(&cfg, input);
	if (cfg_ret != CONFIG_TRUE) {
		fprintf(stderr, "Wrong config format\n");
		ret = -EINVAL;
		goto out;
	}

	root = config_root_setting(&cfg);
	tests_node = config_setting_get_member(root, TESTS_TAG);
	if (!tests_node || !config_setting_is_list(tests_node)) {
		fprintf(stderr, "Missing or incorrect tests list\n");
		ret = -EINVAL;
		goto out;
	}

	count = config_setting_length(tests_node);
	for (i = 0; i < count; ++i) {
		node = config_setting_get_elem(tests_node, i);
		if (!node) {
			ret = -EINVAL;
			goto out;
		}

		test_name = config_setting_get_string(node);
		if (!test_name) {
			fprintf(stderr, "Incorrect tests list. Element %d\n", i);
			ret = -EINVAL;
			goto out;
		}

		ind = lookup_test(test_name);
		if (ind < 0) {
			fprintf(stderr, "Test %s not found.\n", test_name);
			ret = -EINVAL;
			goto out;
		}

		selected[ind] = 1;
	}

	for (i = 0; i < ARRAY_SIZE(selected); ++i) {
		if (selected[i])
			continue;

		tests[i].test_func = &test_skipped;
		tests[i].setup_func = NULL;
		tests[i].teardown_func = NULL;
	}
out:
	config_destroy(&cfg);
	return ret;
}

#else

static int apply_test_config(FILE *input)
{
	fprintf(stderr, "Libconfig is not supported\n");
	return -ENOTSUP;
}

#endif /* HAS_LIBCONFIG */

static void print_help()
{
	fprintf(stderr,
		"libusbgx test suit:\n"
		"    --generate-config - generates config to stdout and exit\n"
		"    --use-config - runs test suit using config from stdin\n"
		"    -h --help - print this message\n"
		);
}

int main(int argc, char **argv)
{
	enum {
		GENERATE_CONFIG = 0x01,
		USE_CONFIG = 0x02,
	};

	int options = 0;
	int opt;
	int ret = -EINVAL;

	static struct option long_options[] = {
		{"generate-config", no_argument, 0, 1},
		{"use-config", no_argument, 0, 2},
		{"help", no_argument, 0, 'h'},
		{NULL, 0, 0, 0}
	};

	while (1) {
		opt = getopt_long(argc, argv, "h", long_options, NULL);
		if (opt < 0)
			break;

		switch (opt) {
		case 1:
			options |= GENERATE_CONFIG;
			break;
		case 2:
			options |= USE_CONFIG;
			break;
		case 'h':
		default:
			print_help();
			goto out;
		}
	}

	if (optind < argc ||
	    ((options & GENERATE_CONFIG) &&
	     (options & USE_CONFIG))) {
		print_help();
		goto out;
	}

	if (options & GENERATE_CONFIG) {
		ret = gen_test_config(stdout);
		goto out;
	}

	if (options & USE_CONFIG) {
		ret = apply_test_config(stdin);
		if (ret)
			goto out;
	}

	ret = cmocka_run_group_tests(tests, NULL, NULL);

out:
	return ret;
}
