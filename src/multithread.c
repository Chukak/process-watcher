#include "multithread.h"

#include <stdlib.h>
#include <time.h>
#include <math.h>

static void set_timeout_from_now(long int offsetms, time_t *sec, long *nsec)
{
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);

  *sec = now.tv_sec + (offsetms / 1000);             // sec
  *nsec = now.tv_nsec + (offsetms % 1000) * 1000000; // convert to nanosec

  *sec += *nsec / (1000000 * 1000); // add sec, if nanosec > (1e9)
  *nsec %= (1000000 * 1000);
}

Condition_variable *Condition_variable_init()
{
  Condition_variable *cv = malloc(sizeof(Condition_variable));
  ASSERT(cv != NULL, "cv (Condition_variable*) != NULL; malloc(...) returns NULL.");
  cv->Timeout_ms = 0;
#ifdef __linux__
  {
    pthread_condattr_t cvattr;
    pthread_condattr_init(&cvattr);
    pthread_condattr_setclock(&cvattr, CLOCK_REALTIME);
    pthread_cond_init(&(cv->__cv), &cvattr);
  }
  cv->__ts = NULL;
  {
    pthread_mutexattr_t mutattr;
    pthread_mutexattr_init(&mutattr);
    pthread_mutexattr_settype(&mutattr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&(cv->__mut), &mutattr);
  }
#elif _WIN32
  // TODO:
#endif
  return cv;
}

void Condition_variable_set_time(Condition_variable *cv, long int ms)
{
  if (!cv->__ts) {
#ifdef __linux__
    cv->__ts = malloc(sizeof(struct timespec));
#elif _WIN32
    // TODO:
#endif
  }
  cv->Timeout_ms = ms;
}

void Condition_variable_wait(Condition_variable *cv)
{
#ifdef __linux__
  if (cv->__ts && cv->Timeout_ms > 0) {
    time_t sec;
    long nsec;
    set_timeout_from_now(cv->Timeout_ms, &sec, &nsec);
    cv->__ts->tv_sec = sec;
    cv->__ts->tv_nsec = nsec;

    pthread_cond_timedwait(&cv->__cv, &cv->__mut, cv->__ts);
  } else
    pthread_cond_wait(&cv->__cv, &cv->__mut);
#elif _WIN32
  // TODO:
#endif
}

void Condition_variable_signal(Condition_variable *cv)
{
#ifdef __linux__
  pthread_cond_signal(&cv->__cv);
#elif _WIN32
  // TODO:
#endif
}

void Condition_variable_destroy(Condition_variable *cv)
{
#ifdef __linux__
  free(cv->__ts);

  pthread_mutex_destroy(&cv->__mut);
  pthread_cond_destroy(&cv->__cv);
#elif _WIN32
  // TODO:
#endif
  free(cv);
}
