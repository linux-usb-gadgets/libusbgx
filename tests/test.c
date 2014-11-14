#include <usbg/usbg.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "usbg-test.h"

#define USBG_TEST(name, test, setup, teardown) \
	{"setup "#test, setup, UNIT_TEST_FUNCTION_TYPE_SETUP}, \
	{name, test, UNIT_TEST_FUNCTION_TYPE_TEST}, \
	{"teardown "#test, teardown, UNIT_TEST_FUNCTION_TYPE_TEARDOWN}

/**
 * @brief Simplest udcs names
 * @details Used to go through init when testing other things
 */
static char *simple_udcs[] = {
	"UDC1",
	"UDC2",
	NULL
};

/**
 * @brief Simplest functions names
 * @details Used to go through init when testing other things
 */
static struct test_function simple_funcs[] = {
	{
		.type = F_ECM,
		.instance = "0"
	}, {
		.type = F_ACM,
		.instance = "0"
	},

	TEST_FUNCTION_LIST_END
};

/**
 * @brief All functions types
 * @details When testing with this in state, check if all func types are
 * processed correctly
 */
static struct test_function all_funcs[] = {
	{
		.type = F_SERIAL,
		.instance = "0"
	}, {
		.type = F_ACM,
		.instance = "0"
	}, {
		.type = F_OBEX,
		.instance = "0"
	}, {
		.type = F_ECM,
		.instance = "0"
	}, {
		.type = F_SUBSET,
		.instance = "0"
	}, {
		.type = F_NCM,
		.instance = "0"
	}, {
		.type = F_EEM,
		.instance = "0"
	}, {
		.type = F_RNDIS,
		.instance = "0"
	}, {
		.type = F_PHONET,
		.instance = "0"
	}, {
		.type = F_FFS,
		.instance = "0"
	},

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
	{
		.label = "c",
		.id = 1,
		.bindings = simple_funcs
	},

	TEST_CONFIG_LIST_END
};

/**
 * @brief Configs bound to all avaible function types
 */
static struct test_config all_bindings_confs[] = {
	{
		.label = "c",
		.id = 2,
		.bindings = no_funcs
	}, {
		.label = "c",
		.id = 4,
		.bindings = all_funcs
	},

	TEST_CONFIG_LIST_END
};

/**
 * @brief Simplest gadget
 */
static struct test_gadget simple_gadgets[] = {
	{
		.name = "g1",
		.udc = "UDC1",
		.configs = simple_confs,
		.functions = simple_funcs
	},

	TEST_GADGET_LIST_END
};

/**
 * @brief Gadgets with all avaible functions
 */
static struct test_gadget all_funcs_gadgets[] = {
	{
		.name = "all_funcs_gadget1",
		.udc = "UDC1",
		.configs = all_bindings_confs,
		.functions = all_funcs
	},

	TEST_GADGET_LIST_END
};

/**
 * @brief Simple state
 */
static struct test_state simple_state = {
	.path = "config",
	.gadgets = simple_gadgets,
	.udcs = simple_udcs
};

/**
 * @brief Setup simple state with some gadgets, configs and functions
 */
void setup_simple_state(void **state)
{
	prepare_state(&simple_state);
	*state = &simple_state;
}

/**
 * @brief State with all functions avaible
 */
static struct test_state all_funcs_state = {
	.path = "all_funcs_configfs",
	.gadgets = all_funcs_gadgets,
        .udcs = simple_udcs
};

/**
 * @brief Setup state with all avaible functions
 */
void setup_all_funcs_state(void **state)
{
	prepare_state(&all_funcs_state);
	*state = &all_funcs_state;
}

/**
 * @brief init usbg with given test state
 */
void init_with_state(struct test_state *in, usbg_state **out)
{
	int usbg_ret;

	push_init(in);
	usbg_ret = usbg_init(in->path, out);
	assert_int_equal(usbg_ret, USBG_SUCCESS);
}

/**
 * @brief Tests usbg_get_gadget function with given state
 * @details Check if gadgets are returned correctly
 */
void test_get_gadget(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);
		assert_gadget_equal(g, &st->gadgets[i]);
	}
}

/**
 * @brief Tests usbg_get_gadget with non-existing gadget name
 * @details Check if get_gadget will not find non-existing gadgets and
 * will not cause crash.
 */
void test_get_gadget_fail(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	g = usbg_get_gadget(s, "non-existing-gadget");
	assert_null(g);
}

/**
 * @brief Tests usbg_get_first_gadget function
 * @details Check if gadget returned by get_first_gadget is actually first one
 */
void test_get_first_gadget(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	g = usbg_get_first_gadget(s);
	assert_non_null(g);
	assert_gadget_equal(g, &st->gadgets[0]);
}

/**
 * @brief Tests get_first_gadget with invalid arguments
 */
void test_get_first_gadget_fail(void **state)
{
	usbg_gadget *g;

	g = usbg_get_first_gadget(NULL);
	assert_null(g);
}

/**
 * @brief Tests getting name of gadget
 * @details Check if gadget name is returned correctly
 */
void test_get_gadget_name(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;
	const char *name;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);
		name = usbg_get_gadget_name(g);
		assert_string_equal(name, st->gadgets[i].name);
	}
}

/**
 * @brief Tests getting name length of gadget
 * @details Check if returned name length is equal original
 */
void test_get_gadget_name_len(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;
	int len;
	int expected;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);

		expected = strlen(st->gadgets[i].name);
		len = usbg_get_gadget_name_len(g);
		assert_int_equal(len, expected);
	}
}

/**
 * @brief Tests getting name of gadget with invalid arguments
 * @details Check if trying to get name of wrong (non-existing) gadget
 * will not cause crash, but return NULL as expected.
 */
void test_get_gadget_name_fail(void **state)
{
	const char *name;

	name = usbg_get_gadget_name(NULL);
	assert_null(name);
}

/**
 * @brief Tests copying gadget's name
 * @details Check if copying gadget name copy actual name correctly
 */
void test_cpy_gadget_name(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;
	char name[USBG_MAX_NAME_LENGTH];
	int ret;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);

		ret = usbg_cpy_gadget_name(g, name, USBG_MAX_NAME_LENGTH);
		assert_int_equal(ret, USBG_SUCCESS);
		assert_string_equal(name, st->gadgets[i].name);
	}
}

/**
 * @brief Test copying gadet name with invalid arguments
 * @details Check if trying to copy gadget name into non-existing buffer,
 * or give invalid buffer length, or invalid gadget will be handled by library
 * and return correct error codes
 */
void test_cpy_gadget_name_fail(void **state)
{
	usbg_gadget *g = NULL;
	usbg_state *s = NULL;
	struct test_state *st;
	int i = 0;
	char name[USBG_MAX_NAME_LENGTH];
	int ret;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	for (i = 0; st->gadgets[i].name; i++) {
		g = usbg_get_gadget(s, st->gadgets[i].name);
		assert_non_null(g);

		ret = usbg_cpy_gadget_name(g, name, 0);
		assert_int_equal(ret, USBG_ERROR_INVALID_PARAM);

/* 		It cause total crash of cmocka framework (SIGSEGV), commented
 *  		temporarly to run other tests crrectly, fail test anyway.
 */
/*		ret = usbg_cpy_gadget_name(g, name, -1);
 *		assert_int_equal(ret, USBG_ERROR_INVALID_PARAM);
 */
		fail_msg("Negative buffer length error");

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
void test_init(void **state)
{
	usbg_state *s = NULL;
	struct test_state *st;

	st = (struct test_state *)(*state);
	*state = NULL;

	init_with_state(st, &s);
	*state = s;

	assert_state_equal(s, st);
}

/**
 * @brief cleanup usbg state
 */
void teardown_state(void **state)
{
	usbg_state *s = NULL;

	s = (usbg_state *)(*state);
	if (s != NULL)
		usbg_cleanup(s);

	cleanup_stack();
}

/* Custom macro for defining test with given name and fixed teardown function */
#define USBG_TEST_TS(name, test, setup) \
	USBG_TEST(name, test, setup, teardown_state)

int main(void)
{
	const UnitTest tests[] = {
		/* Check if init was successfull on simple configfs state */
		USBG_TEST("test_init_simple",
			test_init, setup_simple_state),
		/* Check if init was successfull when all avaible functions
		 * are present in configfs */
		USBG_TEST("test_init_all_funcs",
			test_init, setup_all_funcs_state),
		/* Check if simple gadget will be correcty returned */
		USBG_TEST("test_get_gadget_simple",
			test_get_gadget, setup_simple_state),
		/* Check if getting non-existing and wrong gadgets cause
		 * expected failure and error codes are correct */
		USBG_TEST("test_get_gadget_fail_simple",
			test_get_gadget_fail, setup_simple_state),
		/* Check if gadget returned by get_first_gadget
		 * is actually first one */
		USBG_TEST("test_get_first_gadget_simple",
			test_get_first_gadget, setup_simple_state),
		/* Check if getting first gadget from state returns NULL when
		 * invalid parameters are passed */
		unit_test(test_get_first_gadget_fail),
		/* Check if returned gadget name matches expected value */
		USBG_TEST("test_get_gadget_name_simple",
			test_get_gadget_name, setup_simple_state),
		/* Check if returned simple gadget name length matches expected value */
		USBG_TEST("test_get_gadget_name_len_simple",
			test_get_gadget_name_len, setup_simple_state),
		/* Check if trying to get name of invalid gadget
		 * cause expected failure (name is null) */
		unit_test(test_get_gadget_name_fail),
		/* Check if getting simple gadget name into buffer work as expected*/
		USBG_TEST("test_cpy_gadget_name_simple",
			test_cpy_gadget_name, setup_simple_state),
		/* Check if writting gadget name into buffer fail when
		 * invalid parameters are passed */
		USBG_TEST("test_cpy_gadget_name_fail_simple",
			test_cpy_gadget_name_fail, setup_simple_state),
	};

	return run_tests(tests);
}
