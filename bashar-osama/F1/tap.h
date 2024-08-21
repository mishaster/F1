#pragma once

#include <stdio.h>

#define TEST_SUCCESS 0
#define TEST_FAIL 1

// coloring
#define NOT_OK_TXT "\033[0;31mnot ok\033[0m"
#define OK_TXT "\033[0;32mok\033[0m"

#define BASE_COUNTER(name) int base_counter_##name = __COUNTER__
#define TOP_COUNTER(name) int top_counter_##name = __COUNTER__

#define TEST_DEFINE(name) BASE_COUNTER(name)
#define TEST_END(name) TOP_COUNTER(name)

#define TEST_BODY(name, body)         \
	int test_##name(void)         \
	{                             \
		int STEP_COUNTER = 0; \
		body;                 \
		return TEST_SUCCESS;  \
	}                             \
	TEST_END(name);

#define assert_base(expression, description, message)                    \
	do {                                                             \
		(void)__COUNTER__;                                       \
		STEP_COUNTER++;                                          \
		if ((expression)) {                                      \
			printf("%s %d - %s\n", OK_TXT, STEP_COUNTER,     \
			       description);                             \
		} else {                                                 \
			printf("%s %d - %s\n", NOT_OK_TXT, STEP_COUNTER, \
			       description);                             \
			printf("\tmessage: %s\n", message);              \
			return TEST_FAIL;                                \
		}                                                        \
	} while (0)

#define assert_eq(a, b, desc)                                             \
	do {                                                              \
		assert_base((a) == (b), desc, #a " is not equal to " #b); \
	} while (0)

#define assert_neq(a, b, desc)                                        \
	do {                                                          \
		assert_base((a) != (b), desc, #a " is equal to " #b); \
	} while (0)

#define assert_true(expression, desc)                           \
	do {                                                    \
		assert_base((expression), desc,                 \
			    "\"" #expression "\" is not true"); \
	} while (0)

#define assert_false(expression, desc)                           \
	do {                                                     \
		assert_base(!(expression), desc,                 \
			    "\"" #expression "\" is not false"); \
	} while (0)
