#include "cmdargs.h"
#include "process-watcher-globals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const int DEFAULT_REFRESH_TIMEOUT_MS = 1000; // default refresh timeout
static const int INCORRECT_REFRESH_TIMEOUT_MS = -1;

__attribute__((nothrow)) Cmd_args* Cmd_args_init(int argc, char** argv)
{
  Cmd_args* cmdargs = malloc(sizeof(Cmd_args));
  cmdargs->Valid = argc > 1;
  cmdargs->Process_name = NULL;
  cmdargs->Errormsg = NULL;
  cmdargs->Refresh_timeout_ms = INCORRECT_REFRESH_TIMEOUT_MS;

  if (!cmdargs->Valid) {
    print_help();
    return cmdargs;
  } else {
    for (int i = 1; i < argc; ++i) {
      char* arg = argv[i];
      if (strcmp(arg, "-refresh-timeout-ms") == 0) {
        if (i + 1 >= argc) {
          cmdargs->Valid = false;
          strconcat(&cmdargs->Errormsg, 1, SAFE_PASS_VARGS("No the timeout value after '-refresh-timeout-ms' option."));

          break;
        }

        cmdargs->Refresh_timeout_ms = strtol(argv[++i], NULL, 10);
        if (cmdargs->Refresh_timeout_ms == 0) {
          cmdargs->Valid = false;
          cmdargs->Refresh_timeout_ms = INCORRECT_REFRESH_TIMEOUT_MS;
          strconcat(&cmdargs->Errormsg,
                    1,
                    SAFE_PASS_VARGS("Incorrect the timeout value after '-refresh-timeout-ms' option."));

          break;
        }
      } else {
        cmdargs->Process_name = malloc(sizeof(char) * strlen(arg) + 1);
        strcpy(cmdargs->Process_name, arg);
      }
    }
  }

  if (cmdargs->Valid && cmdargs->Process_name == NULL) {
    cmdargs->Valid = false;
    strconcat(&cmdargs->Errormsg, 1, SAFE_PASS_VARGS("Incorrect process name for watching..."));
  }

  if (cmdargs->Valid && cmdargs->Refresh_timeout_ms <= 0)
    cmdargs->Refresh_timeout_ms = DEFAULT_REFRESH_TIMEOUT_MS;

  return cmdargs;
}

__attribute__((nothrow)) void Cmd_args_free(Cmd_args* args)
{
  if (args) {
    free(args->Process_name);
    free(args->Errormsg);
  }

  free(args);
  args = NULL;
}

__attribute__((nothrow)) void print_help()
{
  // clang-format off
  char * helpmsg = NULL;
  strconcat(&helpmsg, (unsigned short)-1 /* any string length */ , SAFE_PASS_VARGS(
    "Usage: ", __BINARY_NAME, " OPTIONS... process-name \n",
    "Show information about the specified process.\n",
    "Arguments. \n",
    "\t-refresh-timeout-ms N                  Timeout to refresh the information about the specified process.",
    "\n"
  ));
  // clang-format on
  printf("%s", helpmsg);
  free(helpmsg);
}
