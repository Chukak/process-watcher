#ifndef PROPS_H
#define PROPS_H

/**
 * @brief DECLFUNC
 * Default function attributes
 */
#define DECLFUNC __attribute__((nothrow))
/**
 * @brief ATTR
 * User-defined function attributes
 */
#define ATTR(at) __attribute__((at))

/**
 * @brief UNUSED
 * Macro makes the variable unused.
 */
#define UNUSED(__var) (void) __var;

#endif // PROPS_H
