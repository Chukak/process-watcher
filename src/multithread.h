#ifndef __MULTITHREAD_H
#define __MULTITHREAD_H

#include "props.h"
#include <stdbool.h>
#ifdef __linux__
#include <pthread.h>
#elif _WIN32
// TODO:
#endif

/**
 * @brief Condition_variable
 * Implemets the condition variable type using pthread library. This structure can be used to synchronize threads
 * in this code.
 */
typedef struct
{
  long int Timeout_ms; //! waiting timeout
  // private fields
  pthread_cond_t __cv;   // condition variable
  pthread_mutex_t __mut; // mutex
#ifdef __linux__
  struct timespec *__ts; // absolute time with timeout
#elif _WIN32
  // TODO:
#endif
} Condition_variable;

/**
 * @brief Condition_variable_init
 * Initializes the new Condition_variable structure.
 * @return
 */
DECLFUNC Condition_variable *Condition_variable_init() ATTR(warn_unused_result);
/**
 * @brief Condition_variable_set_time
 * Sets waiting timeout on milliseconds. If this timeout is less than zero, a blocking wait will be use. Otherwise a
 * timed wait will be use.
 * @param cv The pointer to the structure
 * @param ms Timeout in milliseconds
 */
DECLFUNC void Condition_variable_set_time(Condition_variable *cv, long int ms) ATTR(nonnull(1));
/**
 * @brief Condition_variable_wait
 * Waiting for the signal of condition variable.
 * @param cv The pointer to the structure
 */
DECLFUNC void Condition_variable_wait(Condition_variable *cv) ATTR(nonnull(1));
/**
 * @brief Condition_variable_signal
 * Signal for this condition variable.
 * @param cv The pointer to the structure
 */
DECLFUNC void Condition_variable_signal(Condition_variable *cv) ATTR(nonnull(1));
/**
 * @brief Condition_variable_destroy
 * Deletes the Condition variable structure.
 * @param cv The pointer to the structure
 */
DECLFUNC void Condition_variable_destroy(Condition_variable *cv) ATTR(nonnull(1));

#endif // __MULTITHREAD_H
