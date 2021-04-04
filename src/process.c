#include "process.h"

#include "ioutils.h"

#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include <signal.h>

#ifdef __linux__
#include <libgen.h>
#include <unistd.h>
#include <linux/limits.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#elif _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <Psapi.h>
#include <sysinfoapi.h>
#endif

#define MAX(a, b) (a > b ? a : b)

#ifdef __linux__
static const char* SYSTEM_PATH_SEPARATOR = "/"; // only linux

static const char* PROC_DIRECTORY_PATH = "/proc";
static const char* CMDLINE_FILENAME = "cmdline";
static const char* EXE_FILENAME = "exe";
static const char* STAT_FILENAME = "stat";
static const char* IO_FILENAME = "io";

static const int PAGESIZE_DIV_VALUE = 1024;
#endif

#define STATE_BUFFER_SIZE 256

#ifdef _WIN32
#define TOKEN_INFORMATION_SIZE 512
#endif

static const char* TIME_FORMAT = "%02d:%02d:%02d";
static const int TIME_STR_LENGTH = 8;

#ifdef _WIN32
static unsigned long long ft2ull(const FILETIME* ft)
{
  ULARGE_INTEGER i;
  i.LowPart = ft->dwLowDateTime;
  i.HighPart = ft->dwHighDateTime;
  return (unsigned long long) i.QuadPart;
}
#endif

static long long monotime_ms()
{
  long long t = 0;
#ifdef __linux__
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  t += ts.tv_sec * 1000;
  t += ts.tv_nsec / 1000000;
#elif _WIN32
  static LARGE_INTEGER freq; // cached frequency
  if (freq.QuadPart == 0)
    QueryPerformanceFrequency(&freq);
  LARGE_INTEGER ts;
  QueryPerformanceCounter(&ts);

  if (freq.QuadPart != 0)
    ts.QuadPart /= freq.QuadPart;

  t += ts.QuadPart * 1000;
#endif
  return t;
}

static double CPU_usage_calculate(unsigned long long utime,
                                  unsigned long long last_utime,
                                  unsigned long long stime,
                                  unsigned long long last_stime,
                                  unsigned long long ttime,
                                  unsigned long long last_ttime)
{
  double totalCoefficient = (double) (ttime - last_ttime);
  if (totalCoefficient != 0)
    return (fabs(100.0 * ((double) (utime - last_utime)) / totalCoefficient) +
            fabs(100.0 * ((double) (stime - last_stime)) / totalCoefficient));
  return 0.0;
}

int pid_by_name(const char* name)
{
  int pid = -1;
#ifdef __linux__
  DIR* proc_dir = opendir(PROC_DIRECTORY_PATH);
  if (proc_dir) {
    struct dirent* dirp;
    while (pid < 0 && (dirp = readdir(proc_dir))) {
      int new_pid = (int) strtol(dirp->d_name, NULL, 10);
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
          char* _ = fgets(data, PATH_MAX, cmdline);
          UNUSED(_);
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
#elif _WIN32
  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);
  // get current snapshot
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0 /* it is ignored */);
  if (Process32First(snapshot, &entry)) { // copy process list
    while (pid == -1 && Process32Next(snapshot, &entry)) {
      // try to find using fullname
      if (strcmp(entry.szExeFile, name) == 0)
        pid = (int) entry.th32ProcessID;
      else { // try to find without .exe suffix
        char* processname = NULL;
        strreplace(entry.szExeFile, &processname, ".exe", "", 1);
        if (strcmp(processname, name) == 0)
          pid = (int) entry.th32ProcessID;
      }
    }
  }
  CloseHandle(snapshot);
#endif
  return pid;
}

Process_stat* Process_stat_init()
{
  Process_stat* stat;

  stat = malloc(sizeof(Process_stat));
  ASSERT(stat != NULL, "stat (Process_stat*) != NULL; malloc(...) returns NULL.");
  stat->Process_name = NULL;
  stat->Pid = -1;
  stat->State = 'U'; // Unknown state

  stat->State_fullname = malloc(sizeof(char) * 7 + 1);
  ASSERT(stat->State_fullname != NULL, "stat->State_fullname (char*) != NULL; malloc(...) returns NULL.");
  strcpy(stat->State_fullname, "Unknown");

  stat->Priority = 0;
  stat->Cpu_usage = 0.0;
  stat->Cpu_peak_usage = 0.0;
  stat->Memory_usage = 0.0;
  stat->Memory_peak_usage = 0.0;

  stat->Start_time = malloc(sizeof(char) * (size_t) TIME_STR_LENGTH + 1);
  ASSERT(stat->Start_time != NULL, "stat->Start_time (char*) != NULL; malloc(...) returns NULL.");
  strcpy(stat->Start_time, "00:00:00");

  stat->Time_usage = malloc(sizeof(char) * (size_t) TIME_STR_LENGTH + 1);
  ASSERT(stat->Time_usage != NULL, "stat->Time_usage (char*) != NULL; malloc(...) returns NULL.");
  strcpy(stat->Time_usage, "00:00:00");

#ifdef __linux__
  stat->Uid = -1;
#endif
  stat->Username = NULL;
  stat->Killed = false;
  stat->Disk_read_mb_usage = 0.0;
  stat->Disk_write_mb_usage = 0.0;
  stat->Disk_read_mb_peak_usage = 0.0;
  stat->Disk_write_mb_peak_usage = 0.0;
  stat->Disk_read_kb = 0;
  stat->Disk_written_kb = 0;

  // private
  stat->__last_utime = 0;
  stat->__last_stime = 0;
  stat->__last_total = 0;
  stat->__last_starttime = 0;
#ifdef __linux__
  stat->__last_btime = 0;
#endif
#ifdef _WIN32
  stat->__phandle = NULL;
#endif
  stat->__last_read_bytes = 0;
  stat->__last_written_bytes = 0;
  stat->__last_monotime = monotime_ms();
  stat->__last_sread_calls = 0;
  stat->__last_swrite_calls = 0;

  return stat;
}

bool Process_stat_set_pid(Process_stat* stat, const char* processname, char** errormsg)
{
  stat->Process_name = malloc(strlen(processname) * sizeof(char) + 1);
  ASSERT(stat->Process_name != NULL, "stat->Process_name (char*) != NULL; malloc(...) returns NULL.");
  strcpy(stat->Process_name, processname);

  int pid;
  if ((pid = pid_by_name(processname)) == -1) {
    strconcat(errormsg,
              3,
              SAFE_PASS_VARGS("Unable to get the information about this process: Pid for '",
                              processname,
                              "' not found!"));
    return false;
  }

  stat->Pid = pid;

#ifdef _WIN32
  stat->__phandle = (HANDLE) OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD) stat->Pid);
  if (!stat->__phandle) { // TODO: show last error
    strconcat(errormsg, 1, SAFE_PASS_VARGS("OpenProcess returns NULL."));
    return false;
  }
#endif
  return true;
}

bool Process_stat_update(Process_stat* pstat, char** errormsg)
{
  char* str_pid = NULL;
  itostr(pstat->Pid, &str_pid);
  bool success = pstat->Pid != -1;
  if (!success) {
    strconcat(errormsg, 5, SAFE_PASS_VARGS("Invalid PID '", str_pid, "' for process '", pstat->Process_name, "'"));
    return false;
  }
#ifdef __linux__
  // common variables
  size_t utimepid, stimepid;
  long int rsspid;
  if (success && !pstat->Killed) {
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
                 "%d " // priority
                 "%*d %*d %*d "
                 "%llu " // starttime
                 "%*d "
                 "%ld " // rss
                 "%*d %*d %*d "
                 "%*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d",
                 &pstat->State,
                 &utimepid,
                 &stimepid,
                 &pstat->Priority,
                 &pstat->__last_starttime,
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
            pstat->Uid = (int) st.st_uid;
            if (pstat->Username)
              free(pstat->Username);

            pstat->Username = malloc(sizeof(char) * strlen(pw->pw_name) + 1);
            strcpy(pstat->Username, pw->pw_name);
          }
        } else {
          pstat->Uid = -1;
          free(pstat->Username);
          pstat->Username = NULL;
        }
        close(fd);
      }
    }

    free(pidstatpath);
    free(pidstatcache);
  }
#elif _WIN32
  if (success && !pstat->Killed) {
    HANDLE token;
    if (!OpenProcessToken((HANDLE) pstat->__phandle, TOKEN_QUERY, &token)) {
      success = false;
      strconcat(errormsg, 3, SAFE_PASS_VARGS("Unable to get the token of process '", pstat->Process_name, "'."));
    } else {
      char tokbuf[TOKEN_INFORMATION_SIZE], username[TOKEN_INFORMATION_SIZE], domain[TOKEN_INFORMATION_SIZE];
      DWORD length, domain_len = TOKEN_INFORMATION_SIZE;
      SID_NAME_USE sid_name;
      if (GetTokenInformation(token, TokenUser, &tokbuf, TOKEN_INFORMATION_SIZE, &length)) {
        PTOKEN_USER ptokuser = (TOKEN_USER*) tokbuf;
        length = TOKEN_INFORMATION_SIZE;

        UNUSED(domain);
        UNUSED(domain_len);
        UNUSED(sid_name);
        if (LookupAccountSidA((LPCSTR) NULL, ptokuser->User.Sid, username, &length, domain, &domain_len, &sid_name)) {
          // TODO: uid?
          pstat->Username = malloc(sizeof(char) * length + 1);
          ASSERT(pstat->Username != NULL, "pstat->Username (char*) != NULL; malloc(...) returns NULL.");
          strcpy(pstat->Username, username);
        } else {
          success = false;
          strconcat(errormsg, 1, SAFE_PASS_VARGS("Unable to get the account information."));
        }
      } else {
        success = false;
        strconcat(errormsg,
                  3,
                  SAFE_PASS_VARGS("Unable to get token information about process '", pstat->Process_name, "'."));
      }
    }

    if (!success) {
      free(pstat->Username);
      pstat->Username = NULL;
    }
    if (token)
      CloseHandle(token);

    if (success) {
      // process status
      DWORD pstatus;
      if (GetExitCodeProcess(pstat->__phandle, &pstatus)) {
        if (pstatus == STILL_ACTIVE)
          pstat->State = 'R';
        else
          pstat->State = 'T'; // stopped
      }
    }
  }
#endif

  if (success) {
    char statestr[STATE_BUFFER_SIZE];
    statestr[0] = '\0';
    switch (pstat->State) {
    case 'U':
      strcpy(statestr, "Unknown");
      break;
    case 'R':
      strcpy(statestr, "Running");
      break;
#ifdef __linux__
    case 'S':
      strcpy(statestr, "Sleeping");
      break;
    case 'Z':
      strcpy(statestr, "Zombie");
      break;
#endif
    case 'T':
      strcpy(statestr, "Stopped");
      break;
    case 'K':
      strcpy(statestr, "Killed by '" __BINARY_NAME "'");
      break;
    }

    if (pstat->State_fullname)
      free(pstat->State_fullname);

    pstat->State_fullname = malloc(sizeof(char) * strlen(statestr) + 1);
    ASSERT(pstat->State_fullname != NULL, "stat->State_fullname (char*) != NULL; malloc(...) returns NULL.");
    strcpy(pstat->State_fullname, statestr);
  }

  if (success && !pstat->Killed) {
#ifdef __linux__
    char* statpath = NULL;
    strconcat(&statpath, 3, SAFE_PASS_VARGS(PROC_DIRECTORY_PATH, SYSTEM_PATH_SEPARATOR, STAT_FILENAME));

    char* statcache = NULL;
    long long statcache_len = fgetall(statpath, &statcache);
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
          pstat->Cpu_usage = CPU_usage_calculate(
              utimepid, pstat->__last_utime, stimepid, pstat->__last_stime, total, pstat->__last_total);

          pstat->Cpu_peak_usage = MAX(pstat->Cpu_peak_usage, pstat->Cpu_usage);

          // save values
          pstat->__last_utime = utimepid;
          pstat->__last_stime = stimepid;
          pstat->__last_total = total;
        }
      }

      // read btime
      char* btime_begin = strstr(statcache, "btime ") + 6 /* btime word length and space */;
      if (btime_begin)
        sscanf(btime_begin, "%llu", &pstat->__last_btime);

      free(statpath);
      free(statcache);
    }
#elif _WIN32
    unsigned long long total_time;
    {
      FILETIME ftime;
      GetSystemTimeAsFileTime(&ftime);
      total_time = ft2ull(&ftime);
      if (total_time == 0) {
        success = false;
        strconcat(errormsg, 1, SAFE_PASS_VARGS("Unable to get the system information for CPU."));
        // TODO: total_time == 0, it is error?
      }
    }

    FILETIME begin_time, end_time, fsys_time, fuser_time;
    if (GetProcessTimes((HANDLE) pstat->__phandle, &begin_time, &end_time, &fsys_time, &fuser_time)) {
      unsigned long long sys_time = ft2ull(&fsys_time), user_time = ft2ull(&fuser_time);
      // TODO: maybe divide this value on num threads/cores
      pstat->Cpu_usage = CPU_usage_calculate(
          user_time, pstat->__last_utime, sys_time, pstat->__last_stime, total_time, pstat->__last_total);

      pstat->Cpu_peak_usage = MAX(pstat->Cpu_peak_usage, pstat->Cpu_usage);

      pstat->__last_utime = user_time;
      pstat->__last_stime = sys_time;
      pstat->__last_total = total_time;

      pstat->__last_starttime = ft2ull(&begin_time) / (unsigned long long) 1e7; // convert to sec
    }
#endif
  }

  if (success && !pstat->Killed) {
    long int mem_usage_kb;
#ifdef __linux__
    // calculate memory usage
    mem_usage_kb = rsspid * (getpagesize() / PAGESIZE_DIV_VALUE);
#elif _WIN32
    {
      PROCESS_MEMORY_COUNTERS pmc;
      GetProcessMemoryInfo(pstat->__phandle, (PPROCESS_MEMORY_COUNTERS) &pmc, sizeof(pmc));
      SYSTEM_INFO info;
      GetSystemInfo(&info);
      // show real working size
      mem_usage_kb = (long int) (pmc.WorkingSetSize /* size in bytes */ / 1024);
      // but, windows task manager show it value
      // mem_usage_kb = (long int)(pmc.PagefileUsage /* size in bytes */ / 1024);
    }
#endif
    pstat->Memory_usage = (double) mem_usage_kb / 1000 + (double) (mem_usage_kb % 1000) / 1000;

    pstat->Memory_peak_usage = MAX(pstat->Memory_peak_usage, pstat->Memory_usage);
  }

  if (success && !pstat->Killed) {
    // calculate time
    struct tm buf;
#ifdef __linux__
    time_t process_starttime =
        (time_t)(pstat->__last_btime /* boot system time in sec */ +
                 (pstat->__last_starttime /
                  (unsigned long long) sysconf(_SC_CLK_TCK) /* kernel planned process time in ticks */));
#elif _WIN32
    time_t process_starttime = (time_t) pstat->__last_starttime; // it is the current local time in seconds!
#endif
#ifdef __linux__
    localtime_r(&process_starttime, &buf);
#elif _WIN32
    localtime_s(&buf, &process_starttime);
#endif
    sprintf(pstat->Start_time, TIME_FORMAT, buf.tm_hour, buf.tm_min, buf.tm_sec);
    pstat->Start_time[TIME_STR_LENGTH] = '\0';
#ifdef __linux__
    time_t process_usagetime = time(0) - process_starttime;
#elif _WIN32
    // windows epoch starts at 1 january 1601
    // unix epoch starts 1 january 1970
    // valid value = 11644473600.0
    time_t sec_to_unix_epoch;
    {
      FILETIME ft;
      // members: {year, month, day of week, day, hour, min, sec, ms}
      SYSTEMTIME st = {1970, 1, 0, 1, 0, 0, 0, 0};
      SystemTimeToFileTime(&st, &ft);
      sec_to_unix_epoch = (time_t)(ft2ull(&ft) / (unsigned long long) 1e7) /* convert to sec*/;
    }
    time_t process_usagetime = time(0) - (process_starttime - sec_to_unix_epoch);
#endif
    // we need duration instead of current localtime
#ifdef __linux__
    gmtime_r(&process_usagetime, &buf);
#elif _WIN32
    gmtime_s(&buf, &process_usagetime);
#endif
    sprintf(pstat->Time_usage, TIME_FORMAT, buf.tm_hour, buf.tm_min, buf.tm_sec);
    pstat->Time_usage[TIME_STR_LENGTH] = '\0';
  }

  if (success && !pstat->Killed) {
    unsigned long long rbytes = 0, // read bytes
        wbytes = 0,                // write bytes
        sysrcalls = 0,             // system read calls count
        syswcalls = 0;             // system write calls count
#ifdef __linux__
    char* pidiopath = NULL;
    strconcat(&pidiopath,
              5,
              SAFE_PASS_VARGS(PROC_DIRECTORY_PATH, SYSTEM_PATH_SEPARATOR, str_pid, SYSTEM_PATH_SEPARATOR, IO_FILENAME));

    char* pidiocache = NULL;
    if (fgetall(pidiopath, &pidiocache) == -1) {
      success = false;
      strconcat(errormsg, 4, SAFE_PASS_VARGS("Unable to open file '", pidiopath, "': ", strerror(errno)));
    } else {
      int args_set = sscanf(pidiocache,
                            "rchar: %llu\n"
                            "wchar: %llu\n"
                            "syscr: %llu\n"
                            "syscw: %llu\n",
                            &rbytes,
                            &wbytes,
                            &sysrcalls,
                            &syswcalls);
      if (args_set != 4) {
        success = false;
        strconcat(errormsg, 3, SAFE_PASS_VARGS("Unable to read data from '/proc/", str_pid, "/io': Invalid order."));
      }
    }
#elif _WIN32
    IO_COUNTERS iocount;
    if (!GetProcessIoCounters(pstat->__phandle, &iocount)) {
      success = false;
      strconcat(
          errormsg,
          5,
          SAFE_PASS_VARGS("Unable to get I/O information for process: ", pstat->Process_name, " (", str_pid, ")."));
    } else {
      rbytes = iocount.ReadTransferCount;
      wbytes = iocount.WriteTransferCount;
      sysrcalls = iocount.ReadOperationCount;
      syswcalls = iocount.WriteOperationCount;
    }
#endif
    if (success) {
      // ms, because we can refresh information every 1 ms.
      double period_ms;
      {
        long long monotime_now = monotime_ms();
        period_ms = (double) (monotime_now - pstat->__last_monotime);
        pstat->__last_monotime = monotime_now;
      }

      pstat->Disk_read_kb = rbytes / 1000;
      pstat->Disk_written_kb = wbytes / 1000;

      // convert to mb/sec
      pstat->Disk_read_mb_usage =
          ((double) (rbytes - pstat->__last_read_bytes) / 1000 / 1000) / (double) (period_ms / 1000);
      if (finite(pstat->Disk_read_mb_usage) == 0)
        pstat->Disk_read_mb_usage = 0.0;

      pstat->Disk_write_mb_usage =
          ((double) (wbytes - pstat->__last_written_bytes) / 1000 / 1000) / (double) (period_ms / 1000);
      if (finite(pstat->Disk_write_mb_usage) == 0)
        pstat->Disk_write_mb_usage = 0.0;

      // skip first update, when all values are zeros
      // TODO: maybe we can find a better desicion
      if (pstat->__last_sread_calls > 0 && pstat->__last_swrite_calls > 0) {
        pstat->Disk_read_mb_peak_usage = MAX(pstat->Disk_read_mb_peak_usage, pstat->Disk_read_mb_usage);
        pstat->Disk_write_mb_peak_usage = MAX(pstat->Disk_write_mb_peak_usage, pstat->Disk_write_mb_usage);
      }

      pstat->__last_read_bytes = rbytes;
      pstat->__last_written_bytes = wbytes;
      pstat->__last_sread_calls = sysrcalls;
      pstat->__last_swrite_calls = syswcalls;
    }
#ifdef __linux__
    free(pidiopath);
    free(pidiocache);
#endif
  }

  free(str_pid);

  return success;
}

void Process_stat_free(Process_stat* stat)
{
  free(stat->Process_name);
  free(stat->State_fullname);
  free(stat->Start_time);
  free(stat->Time_usage);
  free(stat->Username);

  free(stat);
}

bool Process_stat_kill(Process_stat* stat, char** errormsg)
{
  if (stat->Pid > 0) {
    char* str_pid = NULL;
    itostr(stat->Pid, &str_pid);

    int status;
#ifdef __linux__
    status = kill(stat->Pid, SIGKILL);
    if (status != 0) {
#elif _WIN32
    status = TerminateProcess((HANDLE) stat->__phandle, 1);
    if (status == 0) {
#endif
      strconcat(errormsg,
                7,
                SAFE_PASS_VARGS("Unable to kill '", stat->Process_name, "' (", str_pid, "): ", strerror(errno), "."));
      return false;
    }

    stat->Killed = true;
    stat->State = 'K';
    stat->Cpu_usage = 0.0;
    stat->Memory_usage = 0.0;
    stat->Priority = 0;
#ifdef _WIN32
    CloseHandle((HANDLE) stat->__phandle);
#endif
    {
      // length of ' (Killed)' message is 9
      int pname_len = (int) strlen(stat->Process_name);
      if (pname_len > 0) {
        char* allocated = realloc(stat->Process_name, ((size_t) pname_len + 9) * sizeof(char) + 1);
        if (allocated) {
          stat->Process_name = allocated;
          strcat(stat->Process_name, " (Killed)");
        }
      }
    }

    free(str_pid);
  }
  return true;
}
