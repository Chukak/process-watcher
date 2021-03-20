#include "keys.h"

#ifdef __linux__
#include <pthread.h>
#include <unistd.h>
#include <ncurses.h>
#elif _WIN32
#include <Windows.h>
#include <curses.h>
#endif
#include <stdlib.h>
#include <signal.h>

struct __Keys_thread
{
#ifdef __linux__
  pthread_t Thrd;                //! Thread
  pthread_mutex_t Mut;           //! Mutex
  pthread_mutexattr_t Mut_attrs; //! Mutex attributes
#elif _WIN32
  HANDLE Thrd;
  HANDLE Mut;
#endif
#ifdef _MSC_VER // TODO: support atomic
  volatile
#elif __GNUC__ || __MINGW32__
  _Atomic
#endif
      bool Running; //! Thread status
};

DECLFUNC static
#ifdef __linux__
    void *
    process_key(void *arg); // Forward declaration
#elif _WIN32
    DWORD WINAPI process_key(LPVOID);
#endif

DECLFUNC ATTR(warn_unused_result) Keys *Keys_init()
{
  Keys *k = malloc(sizeof(Keys));
  ASSERT(k != NULL, "k (Keys*) != NULL; malloc(...) returns NULL.");
  k->Error_msg = NULL;
  k->Good = true;

  k->__stat = NULL;
  k->__win = NULL;
  k->__thrd = NULL;
  return k;
}

DECLFUNC ATTR(nonnull(1, 2, 3)) void Keys_set_args(Keys *k, Process_stat *stat, Window *win)
{
  k->__stat = stat;
  k->__win = win;
  k->__thrd = malloc(sizeof(struct __Keys_thread));
}

DECLFUNC ATTR(nonnull(1)) void Keys_start_handle(Keys *k)
{
  keypad(k->__win->__p, true);
#ifdef __linux__
  pthread_mutexattr_init(&(k->__thrd->Mut_attrs));
  pthread_mutexattr_settype(&(k->__thrd->Mut_attrs), PTHREAD_MUTEX_NORMAL);

  pthread_mutex_init(&(k->__thrd->Mut), &(k->__thrd->Mut_attrs));
#elif _WIN32
  k->__thrd->Mut = CreateMutexA(NULL, FALSE /* not thread owner */, NULL);
#endif

  k->__thrd->Running = true;
#ifdef __linux__
  pthread_create(&(k->__thrd->Thrd), NULL, process_key, k);
#elif _WIN32
  k->__thrd->Thrd =
      CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) process_key, k, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
#endif
}

DECLFUNC ATTR(nonnull(1)) void Keys_destroy(Keys *k)
{
  k->__thrd->Running = false;
#ifdef __linux__
  pthread_join(k->__thrd->Thrd, NULL);

  pthread_mutexattr_destroy(&(k->__thrd->Mut_attrs));
  pthread_mutex_destroy(&(k->__thrd->Mut));
#elif _WIN32
  WaitForSingleObject(k->__thrd->Thrd, INFINITE);

  CloseHandle(k->__thrd->Mut);
  CloseHandle(k->__thrd->Thrd);
#endif

  free(k->__thrd);

  free(k);
}

DECLFUNC static
#ifdef __linux__
    void *
    process_key(void *arg)
#elif _WIN32
    DWORD WINAPI
    process_key(LPVOID arg)
#endif
{
  Keys *k = (Keys *) arg;
  if (!k)
#ifdef __linux__
    return (void *) -1;
#elif _WIN32
    return FALSE;
#endif

  while (k->__thrd->Running) {
    switch ((int) getch()) {
    case KEY_F(1) /* F1 */: {
#ifdef __linux__
      pthread_mutex_lock(&(k->__thrd->Mut));
#elif _WIN32
      DWORD lock_result = WaitForSingleObject(k->__thrd->Mut, INFINITE);
      switch (lock_result) {
      case WAIT_ABANDONED:
      case WAIT_TIMEOUT:
      case WAIT_FAILED: {
        ASSERT(lock_result == WAIT_OBJECT_0, "Cannot to lock mutex!");
      }
      }
#endif
      if (k->Good && !Process_stat_kill(k->__stat, &k->Error_msg)) {
        k->Good = false;
        printf("%s\n", k->Error_msg);
        printw("%s\n", k->Error_msg);
      }

      Window_refresh(k->__win, k->__stat); // refresh
#ifdef __linux__
      pthread_mutex_unlock(&(k->__thrd->Mut));
#elif _WIN32
      if (!ReleaseMutex(k->__thrd->Mut))
        ASSERT(TRUE != FALSE, "Cannot to unlock mutex!");
#endif
      break;
    }
    case KEY_F(4) /* F4 */:
      raise(SIGINT); // raise SIGINT and exit
#ifdef __linux__
      return (void *) 0;
#elif _WIN32
      return TRUE;
#endif
    }
#ifdef __linux__
    usleep(80 * 1000 /* 80 ms */);
#elif _WIN32
    Sleep(80 /* 80 ms */);
#endif
  }
#ifdef __linux__
  return (void *) 0;
#elif _WIN32
  return TRUE;
#endif
}
