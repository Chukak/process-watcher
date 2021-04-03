#ifndef __PROPS_H
#define __PROPS_H

#include <stdio.h>
#include <assert.h>

/**
 * @brief EXTERNFUNC
 * Marks a function as extern if it is compiled using C++.
 */
#ifdef __cplusplus
#define EXTERNFUNC extern "C"
#else
#define EXTERNFUNC
#endif
/**
 * @brief DECLFUNC
 * Default function attributes
 */
#ifdef __GNUC__
#define DECLFUNC __attribute__((nothrow))
#elif __MINGW32__
#define DECLFUNC __attribute((nothrow))
#elif _MSC_VER
#define DECLFUNC
#endif
/**
 * @brief ATTR
 * User-defined function attributes
 */
#ifdef __GNUC__
#define ATTR(at) __attribute__((at))
#elif __MINGW32__
#define ATTR(at) __attribute((at))
#elif _MSC_VER // TODO: support Visual studio
#define ATTR(at)
#endif
/**
 * @brief UNUSED
 * Macro makes the variable unused.
 */
#define UNUSED(__var) (void) __var;
/**
 * @brief ASSERT
 * Prints the file and the line where the assertion failed.
 */
#define ASSERT(expr, msg)                                                                                              \
  if (!(expr)) {                                                                                                       \
    fprintf(stderr, "assertion failed %s:%d : %s\n", __FILE__, __LINE__, msg);                                         \
    assert(expr);                                                                                                      \
  }

#endif // __PROPS_H
