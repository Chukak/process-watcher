#include "process.h"

#include "process-watcher-globals.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

static const char* SYSTEM_PATH_SEPARATOR = "/"; // only linux

static const char* PROC_DIRECTORY_PATH = "/proc";
static const char* CMDLINE_FILENAME = "cmdline";
static const char* EXE_FILENAME = "exe";
static const char* STAT_FILENAME = "stat";

static const int PAGESIZE_DIV_VALUE = 1024;

static const int CACHE_BUFFER_SIZE = 4096;

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
                SAFE_PASS_VARGS(
                    PROC_DIRECTORY_PATH, SYSTEM_PATH_SEPARATOR, dirp->d_name, SYSTEM_PATH_SEPARATOR, CMDLINE_FILENAME));

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
          strconcat(&exepath,
                    5,
                    SAFE_PASS_VARGS(
                        PROC_DIRECTORY_PATH, SYSTEM_PATH_SEPARATOR, str_pid, SYSTEM_PATH_SEPARATOR, EXE_FILENAME));

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

__attribute__((nothrow)) void Process_stat_init(Process_stat** stat)
{
  *stat = malloc(sizeof(Process_stat));
  (*stat)->Pid = -1;
  (*stat)->State = 'U'; // Unknown state
  (*stat)->Priority = 0;
  (*stat)->Cpu_usage = 0.0;
  (*stat)->Memory_usage = 0.0;
  // private
  (*stat)->__last_utime = 0.0;
  (*stat)->__last_stime = 0.0;
  (*stat)->__last_total = 0.0;
}

__attribute__((nothrow)) bool Process_stat_set_pid(Process_stat** stat, const char* processname, char** errormsg)
{
  if (!(*stat)) {
    char ptrmsg[32];
    sprintf(ptrmsg, "%p", (void*) *stat);
    strconcat(errormsg, 3, SAFE_PASS_VARGS("Process_stat pointer == '", ptrmsg, "'."));
    return false;
  }

  (*stat)->Process_name = malloc(strlen(processname) * sizeof(char) + 1);
  strcpy((*stat)->Process_name, processname);

  int pid;
  if ((pid = pid_by_name(processname)) == -1) {
    strconcat(errormsg,
              3,
              SAFE_PASS_VARGS("Unable to get the information about this process: Pid for '",
                              processname,
                              "' not found!"));
    return false;
  }

  (*stat)->Pid = pid;
  return true;
}

__attribute__((nothrow)) bool Process_stat_update(Process_stat** stat, char** errormsg)
{
  if (!(*stat)) {
    char ptrmsg[32];
    sprintf(ptrmsg, "%p", (const void*) *stat);
    strconcat(errormsg, 3, SAFE_PASS_VARGS("Process_stat pointer == '", ptrmsg, "'."));
    return false;
  }

  char* str_pid = NULL;
  itostr((*stat)->Pid, &str_pid);
  bool success = (*stat)->Pid != -1;
  if (!success) {
    strconcat(errormsg, 5, SAFE_PASS_VARGS("Invalid PID '", str_pid, "' for process '", (*stat)->Process_name, "'"));
    return false;
  }

  // common variables
  size_t utimepid, stimepid;
  long int rsspid;
  if (success) {
    char* pidstatpath = NULL;
    strconcat(
        &pidstatpath,
        5,
        SAFE_PASS_VARGS(PROC_DIRECTORY_PATH, SYSTEM_PATH_SEPARATOR, str_pid, SYSTEM_PATH_SEPARATOR, STAT_FILENAME));

    char cache[CACHE_BUFFER_SIZE];
    FILE* pidstatfile = fopen(pidstatpath, "r");
    if (pidstatfile) {
      fgets(cache, CACHE_BUFFER_SIZE - 1, pidstatfile);
      fclose(pidstatfile);
    } else {
      success = false;
      strconcat(errormsg, 4, SAFE_PASS_VARGS("Unable to open file '", pidstatpath, "': ", strerror(errno)));
    }

    // %*d - skip
    // count all variables in file - 52
    // https://man7.org/linux/man-pages/man5/proc.5.html
    int args_set =
        sscanf(cache,
               "%*d "
               "%*s "
               "%c " // state
               "%*d %*d %*d %*d %*d %*d %*d %*d %*d %*d "
               "%lu " // utime
               "%lu " // stime
               "%*d %*d "
               "%ld " // priority
               "%*d %*d %*d %*d %*d "
               "%ld " // rss
               "%*d %*d %*d "
               "%*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d",
               &(*stat)->State,
               &utimepid,
               &stimepid,
               &(*stat)->Priority,
               &rsspid);
    if (args_set != 5) {
      success = false;
      strconcat(errormsg, 3, SAFE_PASS_VARGS("Unable to read data from '/proc/", str_pid, "/stat': Invalid order."));
    }

    free(pidstatpath);
  }

  if (success) {
    char* statpath = NULL;
    strconcat(&statpath, 3, SAFE_PASS_VARGS(PROC_DIRECTORY_PATH, SYSTEM_PATH_SEPARATOR, STAT_FILENAME));

    char cache[CACHE_BUFFER_SIZE];
    FILE* statfile = fopen(statpath, "r");
    if (statfile) {
      fgets(cache, CACHE_BUFFER_SIZE - 1, statfile);
      fclose(statfile);
    } else {
      success = false;
      strconcat(errormsg, 4, SAFE_PASS_VARGS("Unable to open file '", statpath, "': ", strerror(errno)));
    }
    // we need only the first line
    char* line = strtok(cache, "\r\n");
    size_t line_len = strlen(line);
    if (line_len) {
      size_t total = 0;
      char* sub = strtok(line, " ");
      while (sub != NULL) {
        total += strtoul(sub, NULL, 10);
        sub = strtok(NULL, " ");
      }

      // calculate cpu usage
      double totalCoefficient = total - (*stat)->__last_total;
      if (totalCoefficient != 0)
        (*stat)->Cpu_usage = (fabs(100.0 * (utimepid - (*stat)->__last_utime) / totalCoefficient) +
                              fabs(100.0 * (stimepid - (*stat)->__last_stime) / totalCoefficient));
      else
        (*stat)->Cpu_usage = 0;

      // save values
      (*stat)->__last_utime = utimepid;
      (*stat)->__last_stime = stimepid;
      (*stat)->__last_total = total;
    } else {
      success = false;
      strconcat(errormsg, 3, SAFE_PASS_VARGS("Invalid data in the file '", statfile, "'"));
    }

    free(statpath);
  }

  if (success) {
    // calculate memory usage
    long int mem_usage_kb = rsspid * (getpagesize() / PAGESIZE_DIV_VALUE);
    (*stat)->Memory_usage = (double) mem_usage_kb / 1000 + (double) (mem_usage_kb % 1000) / 1000;
  }

  free(str_pid);

  return success;
}

__attribute__((nothrow)) void Process_stat_free(Process_stat** stat)
{
  if (*stat) {
    free((*stat)->Process_name);
  }
  free(*stat);
  *stat = NULL;
}
