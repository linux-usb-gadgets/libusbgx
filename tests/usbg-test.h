#include <usbg/usbg.h>
#include <libconfig.h>
#include <sys/queue.h>
#include "usbg/usbg_internal.h"

/* Simple structures for defining gadgets. All arrays should be null-terminated.*/

struct test_function
{
	usbg_function_type type;
	char *instance;

	char *path;
	char *name;
};

struct test_config
{
	char *label;
	int id;
	struct test_function *bindings;

	char *path;
	char *name;
};

struct test_gadget
{
	char *name;
	char *udc;
	struct test_config *configs;
	struct test_function *functions;

	char *path;
};

struct test_state
{
	char *configfs_path;
	/* filled by prepare_state() */
	char *path;
	struct test_gadget *gadgets;
	char **udcs;
};

#define TEST_FUNCTION_LIST_END { \
		.instance = NULL, \
	}

#define TEST_CONFIG_LIST_END { \
		.label = NULL, \
		.bindings = NULL, \
	}

#define TEST_GADGET_LIST_END { \
		.name = NULL, \
		.udc = NULL, \
		.configs = NULL, \
		.functions = NULL, \
	}

/**
 * @brief Prepare given state for using in tests
 * @details Generate full pathes to state elements and sort state's content.
 * Must be called before pasing state to push_* and pull_* functions.
 * @param[in] state Pointer to state which should be filled out
 */
void prepare_state(struct test_state *state);

/**
 * @brief Prepare given config for using in tests
 * @details Generate required pathes for given config and sort content
 * (i.e. binding list)
 * @param[in] c Config to be filled out
 * @param[in] path Path to configs directory
 */
void prepare_config(struct test_config *c, char *path);

/**
 * @brief Prepare given function for using in tests
 * @details Generate required pathes for given function
 * @param[in] f Function to be filled out
 * @param[in] path Path to functions directory
 */
void prepare_function(struct test_function *f, char *path);

/**
 * @brief Prepare given gadget for using in tests
 * @details Generate required paths for given gadget and sort it's content
 * (i.e. functions list and config list)
 * @param[in] state Pointer to gadget's parent state
 * @param[in] g Gadget to be filled out
 */
void prepare_gadget(struct test_state *state, struct test_gadget *g);

/**
 * @brief Prepare fake filesystem to init usbg with given test state
 * @details Use wrapped i/o functions to simulate configfs state for usbg.
 * Calling usbg_init without preparation and with mocked i/o functions
 * may fail.
 * @param[in] state Fake state of configfs defined in test
 */
void push_init(struct test_state *state);

/**
 * @brief Store given pointer on cleanup stack
 * @details All stacked pointers will be freed by calling cleanup_queue.
 * This can be used to manage memory needed for single test casees.
 */
void free_later(void *ptr);

/**
 * @brief Cleans up memory no longer needed
 * @details Frees all pointer stored on cleanup stack by calling free_later
 * @warning Calling this function before end of single test usually make test state
 * unusable. Use it only when you no longer need allocated data (at the end of
 * test case, in most cases)
 */
void cleanup_stack();

/**
 * @brief Assert that given usbg function matches given test function
 * @param[in] f Pointer to usbg function struct
 * @param[in] expected Pointer to test function struct with expected values
 */
void assert_func_equal(usbg_function *f, struct test_function *expected);

/**
 * @brief Assert that given usbg config matches given test config
 * @param[in] c Pointer to usbg config struct
 * @param[in] expected Pointer to test config struct with expected values
 */
void assert_config_equal(usbg_config *c, struct test_config *expected);

/**
 * @brief Assert that given usbg gadget matches given test gadget
 * @param[in] g Pointer to usbg gadget struct
 * @param[in] expected Pointer to test gadget struct with expected values
 */
void assert_gadget_equal(usbg_gadget *g, struct test_gadget *expected);

/**
 * @brief Assert that given usbg state matches given test state
 * @param[in] s Pointer to usbg state struct
 * @param[in] expected Pointer to test state struct with expected values
 */
void assert_state_equal(usbg_state *s, struct test_state *expected);

