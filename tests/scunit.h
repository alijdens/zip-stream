/** Simple C Unit Test Framework - Interface.
 *
 *  \author Mariano M. Chouza
 *  \copyright MIT License.
 */

#ifndef SCUNIT_H
#define SCUNIT_H

#include <stdio.h>
#include <string.h>


/** Defines a test with a given ID. A block is expected next. */
#define TEST(id)\
    static void scunit__ ## id ## _test(int *scunit__test_failed);\
    __attribute__((constructor)) void scunit__ ## id ## _reg(void) { scunit_register(scunit__ ## id ## _test, #id, __FILE__); }\
    static void scunit__ ## id ## _test(int *scunit__test_failed)


/** Stringify macro. */
#define STR(s) _STR(s)

/** Stringify auxiliary macro. */
#define _STR(s) #s

/** Basic assertion macro. */
#define GEN_EXP_ASSERT(is_assert, e, true_e, expected)\
    do\
    {\
        if (!(e))\
        {\
            printf("%s failed (" __FILE__ ":" STR(__LINE__) "): '" #true_e "' was expected to be " expected ".\n", (is_assert) ? "Assertion" : "Expectation" );\
            *scunit__test_failed = 1;\
            if (is_assert)\
                return;\
        }\
    } while(0)

/** Assert true macro. */
#define ASSERT_TRUE(e) GEN_EXP_ASSERT(1, e, e, "true")

/** Assert false macro. */
#define ASSERT_FALSE(e) GEN_EXP_ASSERT(1, !(e), e, "false")

/** Assert equal macro. */
#define ASSERT_EQ(expected, e) GEN_EXP_ASSERT(1, (e) == (expected), e, "'" #expected "'")

/** Assert not equal macro. */
#define ASSERT_NE(not_expected, e) GEN_EXP_ASSERT(1, (e) != (not_expected), e, "different from '" #not_expected "'")

/** Expect true macro. */
#define EXPECT_TRUE(e) GEN_EXP_ASSERT(0, e, e, "true")

/** Expect false macro. */
#define EXPECT_FALSE(e) GEN_EXP_ASSERT(0, !(e), e, "false")

/** Expect equal macro. */
#define EXPECT_EQ(expected, e) GEN_EXP_ASSERT(0, (e) == (expected), e, "'" #expected "'")

/** Expect not equal macro. */
#define EXPECT_NE(not_expected, e) GEN_EXP_ASSERT(0, (e) != (not_expected), e, "different from '" #not_expected "'")


/** Test function type. */
typedef void scunit_test_func_t(int *test_failed);


/* prototypes */
void scunit_register(scunit_test_func_t *test_func, const char *test_name, const char *suite_name);

#endif