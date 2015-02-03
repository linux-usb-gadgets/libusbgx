/*
 This is just a temporary fix which should be removed
         when we start building tests with automake
*/
#define HAS_GADGET_SCHEMES
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
#include <libconfig.h>

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
	.configfs_path = "config",
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
	.configfs_path = "all_funcs_configfs",
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
	usbg_ret = usbg_init(in->configfs_path, out);
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

static UnitTest tests[] = {
	/* Check if init was successfull on simple configfs state */
	USBG_TEST_TS("test_init_simple",
		     test_init, setup_simple_state),

	/* Check if init was successfull when all avaible functions
	 * are present in configfs */
	USBG_TEST_TS("test_init_all_funcs",
		     test_init, setup_all_funcs_state),

	/* Check if simple gadget will be correcty returned */
	USBG_TEST_TS("test_get_gadget_simple",
		     test_get_gadget, setup_simple_state),

	/* Check if getting non-existing and wrong gadgets cause
	 * expected failure and error codes are correct */
	USBG_TEST_TS("test_get_gadget_fail_simple",
		     test_get_gadget_fail, setup_simple_state),

	/* Check if gadget returned by get_first_gadget
	 * is actually first one */
	USBG_TEST_TS("test_get_first_gadget_simple",
		     test_get_first_gadget, setup_simple_state),

	/* Check if getting first gadget from state returns NULL when
	 * invalid parameters are passed */
	unit_test(test_get_first_gadget_fail),

	/* Check if returned gadget name matches expected value */
	USBG_TEST_TS("test_get_gadget_name_simple",
		     test_get_gadget_name, setup_simple_state),

	/* Check if returned simple gadget name length matches expected value */
	USBG_TEST_TS("test_get_gadget_name_len_simple",
		     test_get_gadget_name_len, setup_simple_state),

	/* Check if trying to get name of invalid gadget
	 * cause expected failure (name is null) */
	unit_test(test_get_gadget_name_fail),

	/* Check if getting simple gadget name into buffer work as expected*/
	USBG_TEST_TS("test_cpy_gadget_name_simple",
		     test_cpy_gadget_name, setup_simple_state),

	/* Check if writting gadget name into buffer fail when
	 * invalid parameters are passed */
	USBG_TEST_TS("test_cpy_gadget_name_fail_simple",
		     test_cpy_gadget_name_fail, setup_simple_state),
};

#define TESTS_TAG "tests"

int gen_test_config(FILE *output)
{
	config_t cfg;
	config_setting_t *root;
	config_setting_t *tests_node, *node;
	int i;
	int ret = 0, cfg_ret = 0;

	config_init(&cfg);
	config_set_tab_width(&cfg, 4);

	root = config_root_setting(&cfg);
	tests_node = config_setting_add(root, TESTS_TAG, CONFIG_TYPE_LIST);
	if (!tests_node) {
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		if (tests[i].function_type != UNIT_TEST_FUNCTION_TYPE_TEST)
			continue;

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

int lookup_test(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(tests); ++i)
		if (tests[i].function_type == UNIT_TEST_FUNCTION_TYPE_TEST &&
		    !strcmp(name, tests[i].name))
			return i;

	return -1;
}

int apply_test_config(FILE *input)
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

	/* Structures with NULL function are skipped by cmocka*/
	for (i = 0; i < ARRAY_SIZE(selected); ++i) {
		if (selected[i] ||
		    tests[i].function_type != UNIT_TEST_FUNCTION_TYPE_TEST)
			continue;

		if (i - 1 >= 0 && tests[i - 1].function_type ==
		    UNIT_TEST_FUNCTION_TYPE_SETUP)
			tests[i - 1].function = NULL;

		tests[i].function = NULL;

		if (i + 1 < ARRAY_SIZE(tests) && tests[i + 1].function_type ==
		    UNIT_TEST_FUNCTION_TYPE_TEARDOWN)
			tests[i + 1].function = NULL;
	}
out:
	config_destroy(&cfg);
	return ret;
}

void print_skipped_tests(FILE *stream)
{
	int i = 0, nmb_skipped = 0;

	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		if (tests[i].function ||
		    tests[i].function_type != UNIT_TEST_FUNCTION_TYPE_TEST)
			continue;
		++nmb_skipped;
	}

	if (nmb_skipped == 0)
		return;

	fprintf(stream, "[==========] %d test(s) skipped.\n",
		nmb_skipped);

	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		if (tests[i].function ||
		    tests[i].function_type != UNIT_TEST_FUNCTION_TYPE_TEST)
			continue;

		fprintf(stream, "[ SKIPPED  ] %s\n", tests[i].name);
	}
}

void print_help()
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

	ret = run_tests(tests);
	print_skipped_tests(stderr);

out:
	return ret;
}
