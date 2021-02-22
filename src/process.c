#include "process.h"

#include "system-watcher-globals.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdio.h>

static const char* SYSTEM_PATH_SEPARATOR = "/"; // only linux

static const char* PROC_DIRECTORY_PATH = "/proc";
static const char* CMDLINE_FILENAME = "cmdline";
static const char* EXE_FILENAME = "exe";

__attribute__((nothrow)) int pid_by_name(const char* name)
{
  int pid = -1;

  DIR* proc_dir = opendir(PROC_DIRECTORY_PATH);
  if (proc_dir) {
    struct dirent* dirp;
    while (pid < 0 && (dirp = readdir(proc_dir))) {
      int new_pid = strtol(dirp->d_name, NULL, 10);
      if (new_pid <= 0)
        continue;

      char* fullpath = NULL;
      // path /proc/{pid}/cmdline
      strconcat(&fullpath,
                5,
                PROC_DIRECTORY_PATH,
                SYSTEM_PATH_SEPARATOR,
                dirp->d_name,
                SYSTEM_PATH_SEPARATOR,
                CMDLINE_FILENAME);

      char data[PATH_MAX]; // data from cmdline
      {
        FILE* cmdline = fopen(fullpath, "r");
        if (cmdline) {
          fgets(data, PATH_MAX, cmdline);
          fclose(cmdline);
        }
      }
      if (strlen(data) > 0) {
        if (strcmp(name, basename(data)) == 0 || strcmp(name, data) == 0) {
          pid = new_pid;
        } else {
          // if process name not found in the /proc/{pid}/cmdline file
          // looking for name in the /proc/{pid}/exe symbolic link
          char link[PATH_MAX];
          char *exepath = NULL, *str_pid = NULL;
          itostr(new_pid, &str_pid);
          // path /proc/{pid}/exe
          strconcat(
              &exepath, 5, PROC_DIRECTORY_PATH, SYSTEM_PATH_SEPARATOR, str_pid, SYSTEM_PATH_SEPARATOR, EXE_FILENAME);

          ssize_t bytes = readlink(exepath, link, sizeof(link) - 1);
          if (bytes > 0) {
            link[bytes] = '\0';
            if (strcmp(name, basename(link)) == 0) {
              pid = new_pid;
            }
          }

          free(exepath);
          free(str_pid);
        }
      }
      free(fullpath);
    }
    closedir(proc_dir);
  }
  return pid;
}
