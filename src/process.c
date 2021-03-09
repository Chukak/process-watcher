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
#include <time.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX(a, b) (a > b ? a : b)

static const char* SYSTEM_PATH_SEPARATOR = "/"; // only linux

static const char* PROC_DIRECTORY_PATH = "/proc";
static const char* CMDLINE_FILENAME = "cmdline";
static const char* EXE_FILENAME = "exe";
static const char* STAT_FILENAME = "stat";

static const int PAGESIZE_DIV_VALUE = 1024;

static const int STATE_BUFFER_SIZE = 256;

static const char* TIME_FORMAT = "%02d:%02d:%02d";
static const int TIME_STR_LENGTH = 8;

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
  (*stat)->Process_name = NULL;
  (*stat)->Pid = -1;
  (*stat)->State = 'U'; // Unknown state

  (*stat)->State_fullname = malloc(sizeof(char) * 7 + 1);
  strcpy((*stat)->State_fullname, "Unknown\0");

  (*stat)->Priority = 0;
  (*stat)->Cpu_usage = 0.0;
  (*stat)->Cpu_peak_usage = 0.0;
  (*stat)->Memory_usage = 0.0;
  (*stat)->Memory_peak_usage = 0.0;

  (*stat)->Start_time = malloc(sizeof(char) * TIME_STR_LENGTH + 1);
  strcpy((*stat)->Start_time, "00:00:00\0");

  (*stat)->Time_usage = malloc(sizeof(char) * TIME_STR_LENGTH + 1);
  strcpy((*stat)->Time_usage, "00:00:00\0");

  (*stat)->Uid = -1;
  (*stat)->Username = NULL;
  // private
  (*stat)->__last_utime = 0.0;
  (*stat)->__last_stime = 0.0;
  (*stat)->__last_total = 0.0;
  (*stat)->__last_starttime = 0;
  (*stat)->__last_btime = 0;
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

__attribute__((nothrow)) bool Process_stat_update(Process_stat** pstat, char** errormsg)
{
  if (!(*pstat)) {
    char ptrmsg[32];
    sprintf(ptrmsg, "%p", (const void*) *pstat);
    strconcat(errormsg, 3, SAFE_PASS_VARGS("Process_stat pointer == '", ptrmsg, "'."));
    return false;
  }

  char* str_pid = NULL;
  itostr((*pstat)->Pid, &str_pid);
  bool success = (*pstat)->Pid != -1;
  if (!success) {
    strconcat(errormsg, 5, SAFE_PASS_VARGS("Invalid PID '", str_pid, "' for process '", (*pstat)->Process_name, "'"));
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

    char* pidstatcache = NULL;
    if (fgetall(pidstatpath, &pidstatcache) == -1) {
      success = false;
      strconcat(errormsg, 4, SAFE_PASS_VARGS("Unable to open file '", pidstatpath, "': ", strerror(errno)));
    } else {
      // %*d - skip
      // count all variables in file - 52
      // https://man7.org/linux/man-pages/man5/proc.5.html
      int args_set =
          sscanf(pidstatcache,
                 "%*d "
                 "%*s "
                 "%c " // state
                 "%*d %*d %*d %*d %*d %*d %*d %*d %*d %*d "
                 "%lu " // utime
                 "%lu " // stime
                 "%*d %*d "
                 "%ld " // priority
                 "%*d %*d %*d "
                 "%lu " // starttime
                 "%*d "
                 "%ld " // rss
                 "%*d %*d %*d "
                 "%*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d",
                 &(*pstat)->State,
                 &utimepid,
                 &stimepid,
                 &(*pstat)->Priority,
                 &(*pstat)->__last_starttime,
                 &rsspid);
      if (args_set != 6) {
        success = false;
        strconcat(errormsg, 3, SAFE_PASS_VARGS("Unable to read data from '/proc/", str_pid, "/stat': Invalid order."));
      }

      {
        int fd = open(pidstatpath, O_RDONLY);
        struct stat st;
        if (fstat(fd, &st) == 0) {
          struct passwd* pw = getpwuid(st.st_uid);
          if (pw) {
            (*pstat)->Uid = st.st_uid;
            if ((*pstat)->Username)
              free((*pstat)->Username);

            (*pstat)->Username = malloc(sizeof(char) * strlen(pw->pw_name) + 1);
            strcpy((*pstat)->Username, pw->pw_name);
          }
        } else {
          (*pstat)->Uid = -1;
          free((*pstat)->Username);
          (*pstat)->Username = NULL;
        }
        close(fd);
      }
    }

    free(pidstatpath);
    free(pidstatcache);
  }

  if (success) {
    char statestr[STATE_BUFFER_SIZE];
    switch ((*pstat)->State) {
    case 'U':
      strcpy(statestr, "Unknown");
      break;
    case 'R':
      strcpy(statestr, "Running");
      break;
    case 'S':
      strcpy(statestr, "Sleeping");
      break;
    case 'Z':
      strcpy(statestr, "Zombie");
      break;
    case 'T':
      strcpy(statestr, "Stopped");
      break;
    }

    if ((*pstat)->State_fullname)
      free((*pstat)->State_fullname);

    (*pstat)->State_fullname = malloc(sizeof(char) * strlen(statestr) + 1);
    strcpy((*pstat)->State_fullname, statestr);
  }

  if (success) {
    char* statpath = NULL;
    strconcat(&statpath, 3, SAFE_PASS_VARGS(PROC_DIRECTORY_PATH, SYSTEM_PATH_SEPARATOR, STAT_FILENAME));

    char* statcache = NULL;
    int statcache_len = fgetall(statpath, &statcache);
    if (statcache_len == -1) {
      success = false;
      strconcat(errormsg, 4, SAFE_PASS_VARGS("Unable to open file '", statpath, "': ", strerror(errno)));
    } else {
      // calculate cpu
      char tmp[statcache_len + 1];
      strcpy(tmp, statcache); // copy cache

      // we need only the first line
      char* line = strtok(tmp, "\r\n");
      size_t line_len = strlen(line);
      if (line_len) {
        size_t total = 0;
        char* sub = strtok(line, " ");
        while (sub != NULL) {
          total += strtoul(sub, NULL, 10);
          sub = strtok(NULL, " ");
        }

        if (total <= 0) {
          success = false;
          strconcat(errormsg, 3, SAFE_PASS_VARGS("Invalid data in the file '", statpath, "'"));
        } else {
          // calculate cpu usage
          double totalCoefficient = total - (*pstat)->__last_total;
          if (totalCoefficient != 0)
            (*pstat)->Cpu_usage = (fabs(100.0 * (utimepid - (*pstat)->__last_utime) / totalCoefficient) +
                                   fabs(100.0 * (stimepid - (*pstat)->__last_stime) / totalCoefficient));
          else
            (*pstat)->Cpu_usage = 0;

          (*pstat)->Cpu_peak_usage = MAX((*pstat)->Cpu_peak_usage, (*pstat)->Cpu_usage);

          // save values
          (*pstat)->__last_utime = utimepid;
          (*pstat)->__last_stime = stimepid;
          (*pstat)->__last_total = total;
        }
      }

      // read btime
      char* btime_begin = strstr(statcache, "btime ") + 6 /* btime word length and space */;
      if (btime_begin)
        sscanf(btime_begin, "%lu", &(*pstat)->__last_btime);

      free(statpath);
      free(statcache);
    }
  }

  if (success) {
    // calculate memory usage
    long int mem_usage_kb = rsspid * (getpagesize() / PAGESIZE_DIV_VALUE);
    (*pstat)->Memory_usage = (double) mem_usage_kb / 1000 + (double) (mem_usage_kb % 1000) / 1000;

    (*pstat)->Memory_peak_usage = MAX((*pstat)->Memory_peak_usage, (*pstat)->Memory_usage);
  }

  if (success) {
    // calculate time
    struct tm buf;

    time_t process_starttime = (*pstat)->__last_btime + (*pstat)->__last_starttime / sysconf(_SC_CLK_TCK);
    localtime_r(&process_starttime, &buf);
    sprintf((*pstat)->Start_time, TIME_FORMAT, buf.tm_hour, buf.tm_min, buf.tm_sec);
    (*pstat)->Start_time[TIME_STR_LENGTH] = '\0';

    time_t process_usagetime = time(0) - process_starttime;
    // we need duration instead of current localtime
    gmtime_r(&process_usagetime, &buf);
    sprintf((*pstat)->Time_usage, TIME_FORMAT, buf.tm_hour, buf.tm_min, buf.tm_sec);
    (*pstat)->Time_usage[TIME_STR_LENGTH] = '\0';
  }

  free(str_pid);

  return success;
}

__attribute__((nothrow)) void Process_stat_free(Process_stat** stat)
{
  if (*stat) {
    free((*stat)->Process_name);
    free((*stat)->State_fullname);
    free((*stat)->Start_time);
    free((*stat)->Time_usage);
    free((*stat)->Username);
  }
  free(*stat);
  *stat = NULL;
}
