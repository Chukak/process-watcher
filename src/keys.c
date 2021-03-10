#include "keys.h"

#include <pthread.h>
#include <ncurses.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

struct __Keys_thread
{
  pthread_t Thrd;                //! Thread
  pthread_mutex_t Mut;           //! Mutex
  pthread_mutexattr_t Mut_attrs; //! Mutex attributes
  _Atomic bool Running;          //! Thread status
};

__attribute__((nothrow)) static void *process_key(void *arg); // Forward declaration

__attribute__((nothrow)) Keys *Keys_init()
{
  Keys *k = malloc(sizeof(Keys));
  k->Error_msg = NULL;
  k->Good = true;

  k->__stat = NULL;
  k->__win = NULL;
  k->__thrd = NULL;
  return k;
}

__attribute__((nothrow)) void Keys_set_args(Keys *k, Process_stat *stat, Window *win)
{
  if (k) {
    k->__stat = stat;
    k->__win = win;
    k->__thrd = malloc(sizeof(struct __Keys_thread));
  }
}

__attribute__((nothrow)) void Keys_start_handle(Keys *k)
{
  if (k) {
    keypad(k->__win->__p, true);

    pthread_mutexattr_init(&(k->__thrd->Mut_attrs));
    pthread_mutexattr_settype(&(k->__thrd->Mut_attrs), PTHREAD_MUTEX_NORMAL);

    pthread_mutex_init(&(k->__thrd->Mut), &(k->__thrd->Mut_attrs));

    k->__thrd->Running = true;

    pthread_create(&(k->__thrd->Thrd), NULL, process_key, k);
  }
}

__attribute__((nothrow)) void Keys_destroy(Keys *k)
{
  if (k) {
    k->__thrd->Running = false;
    pthread_join(k->__thrd->Thrd, NULL);

    pthread_mutexattr_destroy(&(k->__thrd->Mut_attrs));
    pthread_mutex_destroy(&(k->__thrd->Mut));

    free(k->__thrd);
  }

  free(k);
  k = NULL;
}

__attribute__((nothrow)) static void *process_key(void *arg)
{
  Keys *k = (Keys *) arg;
  if (!k)
    return (void *) -1;

  while (k->__thrd->Running) {
    switch ((int) getch()) {
    case KEY_F(1) /* F1 */: {
      pthread_mutex_lock(&(k->__thrd->Mut));

      if (k->Good && !Process_stat_kill(&(k->__stat), &k->Error_msg)) // TODO print errormsg
        k->Good = false;

      Window_refresh(k->__win, k->__stat); // refresh

      pthread_mutex_unlock(&(k->__thrd->Mut));
      break;
    }
    case KEY_F(4) /* F4 */:
      raise(SIGINT); // raise SIGINT and exit
      return (void *) 0;
    }

    usleep(80 * 1000 /* 80 ms */);
  }
  return (void *) 0;
}
