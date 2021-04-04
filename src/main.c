#include "ioutils.h"
#include "twindow.h"
#include "process.h"
#include "keys.h"
#include "cmdargs.h"

#ifdef __linux__
#include <unistd.h>
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
DECLFUNC static void sighandler(int sig)
{
  UNUSED(sig);
  Is_running = false;
}

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

      Keys* keys = Keys_init();
      Window* mainwin = Window_init();

      Keys_set_args(keys, stat, mainwin);
      Keys_start_handle(keys); // start process keys
      while (Is_running) {
        if (!Window_refresh(mainwin, stat))
          break;

          // TODO: instead of Sleep use condition_variable alternatives
#ifdef __linux__
        usleep((unsigned int) (args->Refresh_timeout_ms * 1000)); // usleep takes microseconds
#elif _WIN32
        Sleep((DWORD) args->Refresh_timeout_ms);
#endif
      }

      Is_running = false;
      Window_destroy(mainwin);
      Keys_destroy(keys);
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
