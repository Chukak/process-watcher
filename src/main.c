#include "ioutils.h"
#include "twindow.h"
#include "process.h"
#include "keys.h"
#include "cmdargs.h"
#include "multithreading.h"

#ifdef __linux__
#include <unistd.h>
#include <pthread.h>
#elif _WIN32
#include <Windows.h>
#endif
#include <signal.h>
#include <stdlib.h>

static
#ifdef _MSC_VER // TODO: support atomic
    volatile
#else
    _Atomic
#endif
    bool Is_running = false;

static void sighandler(int sig)
{
  UNUSED(sig);
  Is_running = false;
}

KEYS_DECL_HANDLER(exit_handler, arg);
KEYS_DECL_HANDLER(start_handler, arg);

int main(int argc, char** argv)
{
  UNUSED(argc);
  UNUSED(argv);

  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);

  int rc = 0;
  Cmd_args* args = Cmd_args_init(argc, argv);
  if (args->Valid) {
    Process_stat* stat = Process_stat_init();

    char* errormsg = NULL;
    if (Process_stat_set_pid(stat, args->Process_name, &errormsg)) {
      Is_running = true;

      Condition_variable* maincv = Condition_variable_init();
      Condition_variable_set_time(maincv, args->Refresh_timeout_ms);

      Keys* keys = Keys_init();
      Window* mainwin = Window_init();

      Keys_set_args(keys, stat, mainwin);
      Keys_set_handler(keys, KEYS_ON_START, start_handler, mainwin);
      Keys_set_handler(keys, KEYS_ON_EXIT, exit_handler, maincv);

      Keys_start_handle(keys); // start process keys
      while (Is_running) {
        if (!Window_refresh(mainwin, stat))
          break;

        Condition_variable_wait(maincv);
      }

      Is_running = false;
      Window_destroy(mainwin);
      Keys_destroy(keys);
      Condition_variable_destroy(maincv);
    } else {
      if (errormsg)
        printf("%s\n", errormsg);
    }

    free(errormsg);
    Process_stat_free(stat);
  } else {
    if (args->Errormsg)
      printf("%s\n", args->Errormsg);
  }

  Cmd_args_free(args);

  return rc;
}

KEYS_DECL_HANDLER(exit_handler, arg)
{
  Condition_variable* cv = (Condition_variable*) arg;
  if (!cv)
    return;

  Condition_variable_signal(cv); // signal waiting
}

KEYS_DECL_HANDLER(start_handler, arg)
{
  Window* w = (Window*) arg;
  fflush(stderr);
  if (!w)
    return;

  keypad(w->__p, true); // start keys handle
}
