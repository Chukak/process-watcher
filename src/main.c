#include "process-watcher-globals.h"
#include "window.h"
#include "process.h"
#include "cmdargs.h"
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

static _Atomic bool Is_running = false;
__attribute__((nothrow)) static void sighandler(int sig)
{
  UNUSED(sig);
  Is_running = false;
}

__attribute__((nothrow)) int main(int argc, char** argv)
{
  UNUSED(argc);
  UNUSED(argv);

  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);

  Cmd_args* args = Cmd_args_init(argc, argv);
  if (args->Valid) {
    Process_stat* stat = NULL;
    Process_stat_init(&stat);

    char* errormsg = NULL;
    if (Process_stat_set_pid(&stat, args->Process_name, &errormsg)) {
      Is_running = true;

      Window* mainwin = Window_init();
      while (Is_running) {
        Window_refresh(mainwin, stat);
        usleep(args->Refresh_timeout_ms * 1000); // usleep takes microseconds
      }

      Is_running = false;
      Window_destroy(mainwin);
    } else {
      if (errormsg)
        printf("%s\n", errormsg);
    }

    free(errormsg);
    Process_stat_free(&stat);
  } else {
    if (args->Errormsg)
      printf("%s\n", args->Errormsg);
  }

  Cmd_args_free(args);

  return 0;
}
