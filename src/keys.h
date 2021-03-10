#ifndef __KEYS_H
#define __KEYS_H

#include "window.h"
#include "process.h"

struct __Keys_thread; // Forward declaration

/**
 * @brief Keys
 * Stores the status of keys processing and error, if occurrs.
 */
typedef struct
{
  char *Error_msg; //! Error message
  bool Good;       //! The status of processing keys
  // private fields
  Process_stat *__stat;
  Window *__win;
  struct __Keys_thread *__thrd;
} Keys;

/**
 * @brief Keys_init
 * initializes the new Keys structure with default values.
 * @return The pointer to the structure
 */
DECLFUNC Keys *Keys_init() ATTR(warn_unused_result);
/**
 * @brief Keys_set_args
 * Sets arguments required for the keys processing.
 * @param k The pointer to the Keys structure
 * @param stat The pointer to the Process_stat structure
 * @param win The pointer to the Window structure
 */
DECLFUNC void Keys_set_args(Keys *k, Process_stat *stat, Window *win) ATTR(nonnull(1, 2, 3));
/**
 * @brief Keys_start_handle
 * Starts the keys processing in an another thread.
 * @param k The pointer to the Keys structure
 */
DECLFUNC void Keys_start_handle(Keys *k) ATTR(nonnull(1));
/**
 * @brief Keys_destroy
 * Deletes the Keys structure.
 * @param k The pointer to the Keys structure
 */
DECLFUNC void Keys_destroy(Keys *k) ATTR(nonnull(1));

#endif // __KEYS_H
