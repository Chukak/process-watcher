#ifndef __TESTING_GLOBALS_H
#define __TESTING_GLOBALS_H

#if defined __GNUC__ || defined __MINGW32__

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#ifdef __linux__
#include <unistd.h>
#elif _WIN32
#include <Windows.h>
#endif

#include "props.h"

// detects filename without full path.
#define __DETECT_FILENAME() strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__

// declare test caller
#define __TEST_DECLARE_CALLER(TestName, TestCaseName) ATTR(constructor(10000)) void TestName##_##TestCaseName()
// test function name (not caller!)
#define __TEST_CASE_FUNC_NAME(TestName, TestCaseName)                                                                  \
  DECLFUNC void __##TestName##__##TestCaseName##__(int* __failed, int* __passed)
// return code of the execution of all tests
int __testing_globals_return_code;

/**
 * @brief TEST_CASE
 * Create a test case macro.
 * A test case - some userdefined function, which testing other functions or situations. The first argument - is a
 * general test name, the second argument - is a specific function or situation associated with the given test case. All
 * test cases has a constructor attribute and is executed before the execution main() function.
 * @code
 * TEST_CASE(String, StringConcat) {...}
 * TEST_CASE(String, StringReplace) {...}
 * TEST_CASE(Numbers, IntToString) {...}
 * @endcode
 *
 * This macro includes declarations and definitions of the test function and the calling function.
 */
#define TEST_CASE(TestName, TestCaseName)                                                                              \
  __TEST_CASE_FUNC_NAME(TestName, TestCaseName);                                                                       \
  __TEST_DECLARE_CALLER(TestName, TestCaseName)                                                                        \
  {                                                                                                                    \
    __testing_globals_print_hdr("" #TestName "", "" #TestCaseName "");                                                 \
    int __failed = 0, __passed = 0;                                                                                    \
                                                                                                                       \
    __##TestName##__##TestCaseName##__(&__failed, &__passed);                                                          \
                                                                                                                       \
    __testing_globals_print_test_result(__failed, __passed);                                                           \
  }                                                                                                                    \
  __TEST_CASE_FUNC_NAME(TestName, TestCaseName)
/**
 * @brief PRERUN
 * Declaration of function, that is executed before all tests and before the execution main() function.
 * @code
 * PRERUN(PreRunFunc) {...}
 * @endcode
 */
#define PRERUN(FuncName) ATTR(constructor(1000)) void __##FuncName##_ctor__()
/**
 * @brief POSTRUN
 * Declaration of function, that is executed after the execution main() function.
 * @code
 * POSTRUN(PostRunFunc) {...}
 * @endcode
 */
#define POSTRUN(FuncName) ATTR(destructor(1000)) void __##FuncName##_dtor__()
/**
 * @brief RUN_TESTS
 * Definition the main function with the execution of all tests.
 */
#define RUN_TESTS()                                                                                                    \
  int main(int argc, char** argv)                                                                                      \
  {                                                                                                                    \
    (void) argc;                                                                                                       \
    (void) argv;                                                                                                       \
    return __testing_globals_return_code;                                                                              \
  }

// declaration of available types to print in checks in test cases
typedef enum
{
  __BOOL = 0,
  __CHAR = 1,
  __SHORT_INT = 2,
  __INT = 3,
  __LONG_INT = 4,
  __FLOAT = 6,
  __LONG_DOUBLE = 7,
  __VOID_PTR = 8,
  __UNSIGNED_CHAR = 9,
  __SIGNED_CHAR = 10,
  __UNSIGNED_SHORT_INT = 12,
  __UNSIGNED_INT = 13,
  __UNSIGNED_LONG_INT = 14,
  __DOUBLE = 16,
  __CHAR_PTR = 17,
  __UNKNOWN = 18
} __testing_globals_types_t;
// detect the type of the passed variable
#define __TYPENAME(x)                                                                                                  \
  _Generic((x), \
    _Bool: __BOOL, \
    unsigned char: __UNSIGNED_CHAR, \
    char: __CHAR, \
    signed char: __SIGNED_CHAR, \
    short int: __SHORT_INT, \
    unsigned short int: __UNSIGNED_SHORT_INT, \
    int: __INT, \
    unsigned int: __UNSIGNED_INT, \
    long int: __LONG_INT, \
    unsigned long int: __UNSIGNED_LONG_INT, \
    float: __FLOAT, \
    double: __DOUBLE, \
    long double: __LONG_DOUBLE, \
    char *: __CHAR_PTR, \
    void *: __VOID_PTR, \
    default: __UNKNOWN)

/**
 * @brief __testing_globals_print_hdr
 * @param testname The common test name
 * @param testcasename The test case name
 */
DECLFUNC void __testing_globals_print_hdr(const char* testname, const char* testcasename);
/**
 * @brief __testing_globals_print_test_result
 * @param __failed Pointer to the failed checks in this test case
 * @param __passed Pointer to the passed checks in this test case
 */
DECLFUNC void __testing_globals_print_test_result(int __failed, int __passed);
/**
 * @brief __testing_globals_print_fail_info
 * @param varname1 The first variable name
 * @param varname2 The second variable name
 * @param desc Description of this check
 * @param filename Filename
 * @param line The line, when this check failed
 * @param type Type of variables
 * @param argc Count passed variables
 */
DECLFUNC void __testing_globals_print_fail_info(const char* varname1,
                                                const char* varname2,
                                                const char* desc,
                                                const char* filename,
                                                int line,
                                                __testing_globals_types_t type,
                                                int argc,
                                                ...);

// checks two variables using predicat and prints information about this check.
#define __CHECK_ANY(__arg1, __arg2, __predicat, __desc)                                                                \
  {                                                                                                                    \
    int __line = __LINE__;                                                                                             \
    if (!__predicat) {                                                                                                 \
      (*__failed)++;                                                                                                   \
      extern int __testing_globals_return_code;                                                                        \
      __testing_globals_return_code = 1;                                                                               \
      __testing_globals_print_fail_info(                                                                               \
          "" #__arg1 "", "" #__arg2 "", __desc, __DETECT_FILENAME(), __line, __TYPENAME(__arg1), 2, __arg1, __arg2);   \
    } else {                                                                                                           \
      (*__passed)++;                                                                                                   \
    }                                                                                                                  \
  }
/**
 * @brief CHECK_EQ
 * Compare two variables. If this variables are not equal, prints information about this failed check. Fullname
 * this macro: 'CHECK_EQUAL'.
 * @code
 * CHECK_EQ(1, 1);
 * @endcode
 */
#define CHECK_EQ(arg1, arg2) __CHECK_ANY(arg1, arg2, (arg1 == arg2), "equal")
/**
 * @brief CHECK_NE
 * Compare two variables. If this variables are equal, prints information about this failed check. Fullname this
 * macro: 'CHECK_NOT_EQUAL'.
 * @code
 * CHECK_NE(1, 2);
 * @endcode
 */
#define CHECK_NE(arg1, arg2) __CHECK_ANY(arg1, arg2, (arg1 != arg2), "not equal")
/**
 * @brief CHECK_GT
 * Compare two variables. If the first variables less or equal than the second variable, prints information about
 * this failed check. Fullname this macro: 'CHECK_GREATER_THAN'.
 * @code
 * CHECK_GT(2, 1);
 * @endcode
 */
#define CHECK_GT(arg1, arg2) __CHECK_ANY(arg1, arg2, (arg1 > arg2), "greater than")
/**
 * @brief CHECK_LT
 * Compare two variables. If the first variables greater or equal than the second variable, prints information
 * about this failed check. Fullname this macro: 'CHECK_LESS_THAN'.
 * @code
 * CHECK_LT(1, 2);
 * @endcode
 */
#define CHECK_LT(arg1, arg2) __CHECK_ANY(arg1, arg2, (arg1 < arg2), "less than")
/**
 * @brief CHECK_GE
 * Compare two variables. If the first variables less than the second variable, prints information about
 * this failed check. Fullname this macro: 'CHECK_GREATER_OR_EQUAL'.
 * @code
 * CHECK_GE(2, 2);
 * @endcode
 */
#define CHECK_GE(arg1, arg2) __CHECK_ANY(arg1, arg2, (arg1 >= arg2), "greater or equal")
/**
 * @brief CHECK_LE
 * Compare two variables. If the first variables greater than the second variable, prints information about
 * this failed check. Fullname this macro: 'CHECK_LESS_OR_EQUAL'.
 * @code
 * CHECK_GE(2, 2);
 * @endcode
 */
#define CHECK_LE(arg1, arg2) __CHECK_ANY(arg1, arg2, (arg1 <= arg2), "less or equal")
/**
 * @brief STRINGS_ARE_EQUAL
 * Compares two strings for equality
 */
#define STRINGS_ARE_EQUAL(s1, s2) (strcmp(s1, s2) == 0)
/**
 * @brief STRINGS_ARE_NOT_EQUAL
 * Compares two strings for non-equality
 */
#define STRINGS_ARE_NOT_EQUAL(s1, s2) (strcmp(s1, s2) != 0)
/**
 * @brief CHECK_STR_EQ
 * The same macro as 'CHECK_EQ', only for strings.
 */
#define CHECK_STR_EQ(arg1, arg2) __CHECK_ANY(arg1, arg2, STRINGS_ARE_EQUAL(arg1, arg2), "equal")
/**
 * @brief CHECK_STR_NE
 * The same macro as 'CHECK_NE', only for strings.
 */
#define CHECK_STR_NE(arg1, arg2) __CHECK_ANY(arg1, arg2, STRINGS_ARE_NOT_EQUAL(arg1, arg2), "not equal")

#ifdef __linux__
#define __SLEEP(sec) sleep(sec);
#elif _WIN32
#define __SLEEP(ms) Sleep(ms * 1000);
#endif

#define SLEEP_SEC(n)                                                                                                   \
  {                                                                                                                    \
    printf("Waiting for delay (%d sec)...", n);                                                                        \
    fflush(stdout);                                                                                                    \
    __SLEEP(n);                                                                                                        \
    printf("done!\n");                                                                                                 \
    fflush(stdout);                                                                                                    \
  }

#else
#error "At the moment testing is only available for the GCC and MinGW compilers."
#endif

#endif // __TESTING_GLOBALS_H
