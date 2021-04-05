#ifndef __KEYS_H
#define __KEYS_H

#include "twindow.h"
#include "process.h"
#include "multithread.h"

struct __Keys_thread; // Forward declaration

/**
 * @brief Keys_handler
 * The pointer the the handler.
 */
typedef void (*Keys_handler)(void *);

struct __Keys_handler; // Forward declaration

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
  struct __Keys_handler *__on_start; // hanler on start
  struct __Keys_handler *__on_exit;  // handler on exit
} Keys;

#define KEYS_DECL_HANDLER(name, argname) void name(void *argname)

/**
 * @brief Keys_handler_attr
 * Handler attributes.
 */
typedef enum
{
  KEYS_ON_START,
  KEYS_ON_EXIT
} Keys_handler_attr;

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
 * @brief Keys_set_handler
 * Sets handler with attributes. Handlers may be use on start or exit.
 * @param k The pointer to the Keys structure
 * @param attr Handelr attributes
 * @param f The pointer to the handler
 * @param arg the pointer to the arg
 */
DECLFUNC void Keys_set_handler(Keys *k, Keys_handler_attr attr, Keys_handler f, void *arg) ATTR(nonnull(1, 3, 4));
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
