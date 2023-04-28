/** Simple C Unit Test Framework - Implementation.
 *
 *  \author Mariano M. Chouza
 *  \copyright MIT License.
 */

#define SCUNIT_C
#define _GNU_SOURCE
#include "scunit.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* protects agains redefinition of main */
#ifdef main
    #undef main
#endif


/** Test node type. */
typedef struct scunit_test_node_t
{
    /** Test function. */
    scunit_test_func_t *test_func;

    /** Test name. */
    const char *test_name;

    /** Test suite name. */
    const char *suite_name;

    /** Next node. */
    struct scunit_test_node_t *next;

    /** Failed. */
    int failed;

    /** Dummy order variable (required because qsort() is not stable). */
    int order;

} scunit_test_node_t;


/** Test suite entry type. */
typedef struct
{
    /** Suite name. */
    const char *suite_name;

    /** Number of tests. */
    size_t num_tests;

    /** Tests. */
    scunit_test_node_t **tests;

} scunit_suite_t;


/** Tests list. */
static scunit_test_node_t *_tests_list = NULL;


/** Error ANSI sequence. */
static const char *_err_ansi_seq = "\x1b[31;1m";


/** Success ANSI sequence. */
static const char *_ok_ansi_seq = "\x1b[32;1m";


/** Normal ANSI sequence. */
static const char *_normal_ansi_seq = "\x1b[0m";


/** Gets a monotonic timestamp.
 *
 *  \return Monotonic timestamp in nanoseconds.
 */
static unsigned long long _get_timestamp(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000ull + t.tv_nsec;
}


/** Compares two suites.
 *
 *  \param p1 First suite.
 *  \param p2 Second suite.
 *  \return Comparison value.
 */
static int _cmp_suite(const void *p1, const void *p2)
{
    /* gets the suites */
    const scunit_suite_t *s1 = (const scunit_suite_t *)p1;
    const scunit_suite_t *s2 = (const scunit_suite_t *)p2;

    /* compares the names */
    return strcmp(s1->suite_name, s2->suite_name);
}


/** Compares two tests nodes (assuming same suite).
 *
 *  \param p1 First suite.
 *  \param p2 Second suite.
 *  \return Comparison value.
 */
static int _cmp_test(const void *p1, const void *p2)
{
    /* gets the test nodes */
    const scunit_test_node_t *n1 = *(const scunit_test_node_t **)p1;
    const scunit_test_node_t *n2 = *(const scunit_test_node_t **)p2;

    /* compares the test order values (reverse of normal ordering) */
    return n1->order > n2->order ? -1 : (n1->order < n2->order ? 1 : 0);
}


/** Gets suite name for the file name.
 *
 *  \param filename Suite file name.
 *  \return Suite name.
 *  \note The suite name should be freed by the caller.
 */
static char *_get_suite_name(const char *filename)
{
    /* duplicates the filename */
    char *suite_name = malloc(strlen(filename) + 1);
    strcpy(suite_name, filename);

    /* checks if it needs to remove the suffix */
    char *last_dot_pos = strrchr(suite_name, '.');
    if (last_dot_pos != NULL)
        *last_dot_pos = '\0';

    /* returns the suite name */
    return suite_name;
}


/** Gets suites array.
 *
 *  \param num_suites Number of test suites (output).
 *  \param num_tests Number of tests (output).
 *  \param test_suites Test suites array (output).
 *  \param tests_list Tests list.
 */
static void _get_suites_arr(size_t *num_suites, size_t *num_tests, scunit_suite_t **test_suites, scunit_test_node_t *tests_list)
{
    /* counts the suites & tests */
    const char *prev_suite_name = "";
    *num_suites = 0;
    *num_tests = 0;
    for (scunit_test_node_t *n = tests_list; n != NULL; n = n->next)
    {
        if (strcmp(n->suite_name, prev_suite_name))
        {
            prev_suite_name = n->suite_name;
            (*num_suites)++;
        }
        n->order = *num_tests;
        (*num_tests)++;
    }

    /* builds the suites array */
    *test_suites = calloc(*num_suites, sizeof(scunit_suite_t));

    /* counts the tests per suite and loads the suite names */
    prev_suite_name = "";
    size_t si = 0;
    for (const scunit_test_node_t *n = tests_list; n != NULL; n = n->next)
    {
        if (strcmp(n->suite_name, prev_suite_name))
        {
            prev_suite_name = n->suite_name;
            (*test_suites)[si].suite_name = _get_suite_name(n->suite_name);
            si++;
        }
        (*test_suites)[si-1].num_tests++;
    }

    /* allocates the test arrays */
    for (size_t i = 0; i < *num_suites; i++)
        (*test_suites)[i].tests = calloc((*test_suites)[i].num_tests, sizeof(scunit_test_node_t *));

    /* stores the tests, in sorted order */
    scunit_test_node_t *n = tests_list;
    for (size_t i = 0; i < *num_suites; i++)
    {
        for (size_t j = 0; j < (*test_suites)[i].num_tests; j++)
        {
            (*test_suites)[i].tests[j] = n;
            n = n->next;
        }
        qsort((*test_suites)[i].tests, (*test_suites)[i].num_tests, sizeof(scunit_test_node_t *), _cmp_test);
    }

    /* sorts the suites */
    qsort(*test_suites, *num_suites, sizeof(scunit_suite_t), _cmp_suite);
}


/** Releases suites array.
 *
 *  \param test_suites Test suites array.
 *  \param num_suites Number of test suites.
 */
static void _release_suites_arr(scunit_suite_t *test_suites, size_t num_suites)
{
    for (size_t i = 0; i < num_suites; i++)
    {
        free((void *)test_suites[i].suite_name);
        free((void *)test_suites[i].tests);
    }
    free(test_suites);
}


/** Runs all tests.
 *
 *  \return 0 if all tests succeeded, 1 otherwise.
 */
static int _run_all_tests(void)
{
    /* error code */
    int error_code = 0;
    /* number of suites */
    size_t num_suites = 0;
    /* number of tests */
    size_t num_tests = 0;
    /* suites array */
    scunit_suite_t *suites = NULL;
    /* number of successes */
    size_t num_success = 0;
    /* number of failures */
    size_t num_fail = 0;

    /* gets the suites array */
    _get_suites_arr(&num_suites, &num_tests, &suites, _tests_list);

    /* gets the general start */
    unsigned long long all_tests_start = _get_timestamp();

    /* prints header */
    printf("%s[==========]%s Running %zu test(s) from %zu suite(s).\n", _ok_ansi_seq, _normal_ansi_seq, num_tests, num_suites);

    /* goes over every suite */
    for (size_t i = 0; i < num_suites; i++)
    {
        /* gets the suite start */
        unsigned long long suite_start = _get_timestamp();

        /* prints suite header */
        printf("%s[----------]%s %zu test(s) from %s\n", _ok_ansi_seq, _normal_ansi_seq, suites[i].num_tests, suites[i].suite_name);

        /* goes over every test */
        for (size_t j = 0; j < suites[i].num_tests; j++)
        {
            /* gets the test start */
            unsigned long long test_start = _get_timestamp();

            /* prints the test header */
            printf("%s[ RUN      ]%s %s.%s\n", _ok_ansi_seq, _normal_ansi_seq, suites[i].suite_name, suites[i].tests[j]->test_name);

            /* executes the test */
            int test_error = 0;
            suites[i].tests[j]->test_func(&test_error);

            /* checks if it failed */
            if (test_error != 0)
            {
                /* prints the failure indicator */
                printf("%s[  FAILED  ]%s %s.%s (%llu ms)\n",
                       _err_ansi_seq,
                       _normal_ansi_seq,
                       suites[i].suite_name,
                       suites[i].tests[j]->test_name,
                       (_get_timestamp() - test_start) / 1000000ull);

                /* marks its failure */
                suites[i].tests[j]->failed = 1;

                /* updates the failure counter */
                num_fail++;
            }
            else
            {
                /* prints the success indicator */
                printf("%s[       OK ]%s %s.%s (%llu ms)\n",
                       _ok_ansi_seq,
                       _normal_ansi_seq,
                       suites[i].suite_name,
                       suites[i].tests[j]->test_name,
                       (_get_timestamp() - test_start) / 1000000ull);

                /* updates the success counter */
                num_success++;
            }
        }

        /* prints suite footer */
        printf("%s[----------]%s %zu test(s) from %s (%llu ms total)\n",
               _ok_ansi_seq,
               _normal_ansi_seq,
               suites[i].num_tests,
               suites[i].suite_name,
               (_get_timestamp() - suite_start) / 1000000ull);
    }

    /* prints footer */
    printf("%s[==========]%s %zu test(s) from %zu suite(s) ran (%llu ms total).\n",
           _ok_ansi_seq,
           _normal_ansi_seq,
           num_tests,
           num_suites,
           (_get_timestamp() - all_tests_start) / 1000000ull);
    printf("%s[  PASSED  ]%s %zu test(s).\n", _ok_ansi_seq, _normal_ansi_seq, num_success);
    if (num_fail > 0)
    {
        printf("%s[  FAILED  ]%s %zu test(s), listed below:\n", _err_ansi_seq, _normal_ansi_seq, num_fail);
        for (size_t i = 0; i < num_suites; i++)
            for (size_t j = 0; j < suites[i].num_tests; j++)
                if (suites[i].tests[j]->failed)
                    printf("%s[  FAILED  ]%s %s.%s\n", _err_ansi_seq, _normal_ansi_seq, suites[i].suite_name, suites[i].tests[j]->test_name);
    }

    /* releases the suites array */
    _release_suites_arr(suites, num_suites);

    /* returns the error code */
    return error_code;
}


/** Releases the tests list.
 *
 */
static void _release_tests_list(void)
{
    for (scunit_test_node_t *n = _tests_list; n != NULL; )
    {
        scunit_test_node_t *t = n;
        n = n->next;
        free(t);
    }
}


/** Registers a test to be executed.
 *
 *  \param test_func Test function.
 *  \param test_name Test name.
 *  \param suite_name Suite name.
 */
void scunit_register(scunit_test_func_t *test_func, const char *test_name, const char *suite_name)
{
    /* creates a new node */
    scunit_test_node_t *n = calloc(1, sizeof(scunit_test_node_t));
    n->test_func = test_func;
    n->test_name = test_name;
    n->suite_name = suite_name;

    /* puts it at the beginning of the list */
    n->next = _tests_list;
    _tests_list = n;
}


/** Framework entry point.
 *
 *  \return Program exit code.
 */
int main(void)
{
    /* if stdout is not a terminal, disables the coloring */
    if (isatty(STDOUT_FILENO) != 1)
    {
        _normal_ansi_seq = "";
        _ok_ansi_seq = "";
        _err_ansi_seq = "";
    }

    /* just runs all tests */
    int exit_code = _run_all_tests();

    /* releases the tests list */
    _release_tests_list();

    /* returns the exit code */
    return exit_code;
}