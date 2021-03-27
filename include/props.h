#ifndef __PROPS_H
#define __PROPS_H

#include <stdio.h>
#include <assert.h>

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

#define ASSERT(expr, msg)                                                                                              \
  if (!(expr)) {                                                                                                       \
    fprintf(stderr, "assertion failed %s:%d : %s\n", __FILE__, __LINE__, msg);                                         \
    assert(expr);                                                                                                      \
  }

#endif // __PROPS_H
