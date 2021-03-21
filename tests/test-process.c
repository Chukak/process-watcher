#include "testing-globals.h"

#include "process.h"
#include "process-watcher-globals.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
#include <sys/stat.h>
#elif _WIN32
#include <Windows.h>
#endif
#include <string.h>

DECLFUNC static int
#ifdef __linux__
get_real_pid(const char *name)
#elif _WIN32
get_real_pid(const char *name, HANDLE *phandle)
#endif
{
  int pid = -1;
  char buf[512], command[512];
  sprintf(command,
#ifdef __linux__
          "pidof %s",
#elif _WIN32
          "powershell -command \"(get-process %s).id \" ",
#endif
          name);
  FILE *pipe = popen(command, "r");
  assert(pipe != NULL);
  if (fgets(buf, 512, pipe) != NULL) {
    pclose(pipe);
    // remove spaces
    char *cache = NULL;
    strreplace(buf, &cache, " ", "", -1 /* all */);
    pid = (int) strtol(cache, NULL, 10);
    free(cache);
#ifdef _WIN32
    *phandle = (HANDLE) OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD) pid);
    assert(*phandle != NULL);
#endif
  }
  return pid;
}

DECLFUNC static void compile_binary(const char *name, const char *code)
{
  const char *srcName = "src.c";
  FILE *src = fopen(srcName, "w");
  assert(src != NULL);
  assert(fprintf(src, "%s", code) > 0);
  assert(fclose(src) == 0);

  char command[512];
  sprintf(command,
#if defined __GNUC__ || defined __MINGW32__
          "gcc -o %s src.c",
#else // TODO: other compilers
          ""
#endif
          name);
  assert(system(command) == 0);
#ifdef __linux__
  assert(chmod(srcName, S_IRWXU | S_IRWXO) == 0);
#endif
}

DECLFUNC static void clean_temp_files()
{
  remove("src.c");
}

DECLFUNC static bool
#ifdef __linux__
check_process(int pid)
#elif _WIN32
check_process(HANDLE phandle)
#endif
{
#ifdef __linux__
  return getpgid(pid) >= 0;
#elif _WIN32
  DWORD pstatus;
  if (GetExitCodeProcess(phandle, &pstatus))
    return (pstatus == STILL_ACTIVE);
  return false;
#endif
}

TEST_CASE(Process, GetProcessNameByPid)
{
  // compile test binary
  const char *processCode = "#include <stdio.h>\n"
                            "#ifdef __linux__\n"
                            "#include <unistd.h>\n"
                            "#define SLEEP(n) sleep(n)\n"
                            "#elif _WIN32\n"
                            "#include <Windows.h>\n"
                            "#define SLEEP(n) Sleep(n * 1000)\n"
                            "#endif\n"
                            "int main(){\n"
                            "while(1){printf(\"Test message.\");SLEEP(1);}\n"
                            "}";
  const char *binname =
#ifdef __linux__
      "testprocess";
#elif _WIN32
      "testprocess.exe";
#endif
  compile_binary(binname, processCode);
  {
    {
      char command[256];
      sprintf(command,
#ifdef __linux__
              "./testprocess > /dev/null & 2>&1 && sleep 2"
#elif _WIN32
              "start /B testprocess.exe > NUL 2>&1 && timeout 2 > NUL 2>&1"
#endif
      );

      assert(system(command) == 0);
    }

    int pid = pid_by_name(binname);
    CHECK_NE(pid, -1);
#ifdef _WIN32
    HANDLE phandle;
#endif
    // get the actual PID using pidof command
    int actualPid =
#ifdef __linux__
        get_real_pid(binname);
#elif _WIN32
        get_real_pid("testprocess", &phandle);
#endif
    assert(actualPid > 0);
    // compare PID's
    CHECK_EQ(pid, actualPid);
    CHECK_EQ(
#ifdef __linux__
        check_process(actualPid),
#elif _WIN32
        check_process(phandle),
#endif
        true);

#ifdef __linux__
    if (check_process(actualPid))
      kill(actualPid, SIGKILL);
#elif _WIN32
    if (check_process(phandle))
      assert(TerminateProcess(phandle, 1) != 0);
    CloseHandle(phandle);
#endif
  }
  clean_temp_files();
  remove(binname);
}

TEST_CASE(Process, ProcessStatStructureUsage)
{
  // compile test binary
  const char *processCode = "#include <stdio.h>\n"
                            "#ifdef __linux__\n"
                            "#include <unistd.h>\n"
                            "#define SLEEP(n) sleep(n)\n"
                            "#elif _WIN32\n"
                            "#include <Windows.h>\n"
                            "#define SLEEP(n) Sleep(n * 1000)\n"
                            "#endif\n"
                            "#include <stdlib.h>\n"
                            "int main(){\n"
                            "char *buffer = malloc(8128 * sizeof(char));\n"
                            "while(1){\n"
                            "printf(\"Test message.\");SLEEP(1);\n"
                            "for(long i=0; i < 180000000000; ++i);\n"
                            "}\n"
                            "free(buffer);\n"
                            "}\n";
  const char *binname =
#ifdef __linux__
      "testprocess";
#elif _WIN32
      "testprocess.exe";
#endif
  compile_binary(binname, processCode);
  {
    {
      char command[256];
      sprintf(command,
#ifdef __linux__
              "./testprocess > /dev/null & 2>&1 && sleep 2"
#elif _WIN32
              "start /B testprocess.exe > NUL 2>&1 && timeout 2 > NUL 2>&1"
#endif
      );

      assert(system(command) == 0);
    }
#ifdef _WIN32
    HANDLE phandle;
#endif
    int actualPid =
#ifdef __linux__
        get_real_pid(binname);
#elif _WIN32
        get_real_pid("testprocess", &phandle);
#endif
    assert(actualPid > 0);

    Process_stat *statobj = Process_stat_init();
    CHECK_NE(statobj, NULL);

    char *errormsg = NULL;
    CHECK_EQ(Process_stat_set_pid(statobj, binname, &errormsg), true);
    CHECK_EQ(statobj->Pid, actualPid);
    CHECK_STR_EQ(statobj->Process_name, binname);
    CHECK_EQ(statobj->State, 'U'); // Unknown state
    CHECK_STR_EQ(statobj->State_fullname, "Unknown");
    CHECK_STR_EQ(statobj->Time_usage, "00:00:00");
    CHECK_STR_EQ(statobj->Start_time, "00:00:00");
#ifdef __linux__
    CHECK_EQ(statobj->Uid, -1);
#endif
    CHECK_EQ(statobj->Username, NULL);
    CHECK_EQ(statobj->Killed, false);

    CHECK_EQ(Process_stat_update(statobj, &errormsg), true);
    CHECK_EQ(statobj->Pid, actualPid);

    SLEEP_SEC(8);

    CHECK_EQ(Process_stat_update(statobj, &errormsg), true);
    CHECK_EQ(statobj->Pid, actualPid);
    CHECK_EQ(statobj->State, 'R'); // Running state
    CHECK_GT(statobj->Cpu_usage, 0.0);
    CHECK_GT(statobj->Memory_usage, 0.0);
    CHECK_EQ(statobj->Cpu_peak_usage, statobj->Cpu_usage);
    CHECK_EQ(statobj->Memory_peak_usage, statobj->Memory_usage);
    CHECK_STR_EQ(statobj->State_fullname, "Running");
    CHECK_STR_NE(statobj->Time_usage, "00:00:00");
    CHECK_STR_NE(statobj->Start_time, "00:00:00");
#ifdef __linux__
    CHECK_NE(statobj->Uid, -1);
#endif
    CHECK_NE(statobj->Username, NULL);
    CHECK_EQ(statobj->Killed, false);

    CHECK_EQ(Process_stat_kill(statobj, &errormsg), true);   // kill
    CHECK_EQ(Process_stat_update(statobj, &errormsg), true); // refresh
    CHECK_EQ(statobj->Pid, actualPid);
    CHECK_EQ(statobj->State, 'K'); // Running state
    CHECK_EQ(statobj->Cpu_usage, 0.0);
    CHECK_EQ(statobj->Memory_usage, 0.0);
    CHECK_EQ(statobj->Priority, 0);
    CHECK_NE(statobj->Cpu_peak_usage, statobj->Cpu_usage);
    CHECK_NE(statobj->Memory_peak_usage, statobj->Memory_usage);
    CHECK_STR_EQ(statobj->State_fullname,
                 ""
                 "Killed by '" __BINARY_NAME "'");
#ifdef __linux__
    CHECK_NE(statobj->Uid, -1);
#endif
    CHECK_NE(statobj->Username, NULL);
    CHECK_EQ(statobj->Killed, true);

    SLEEP_SEC(2);

    CHECK_EQ(
#ifdef __linux__
        check_process(actualPid),
#elif _WIN32
        check_process(phandle),
#endif
        false);

    Process_stat_free(statobj);

#ifdef __linux__
    if (check_process(actualPid))
      kill(actualPid, SIGKILL);
#elif _WIN32
    if (check_process(phandle))
      assert(TerminateProcess(phandle, 1) != 0);
    CloseHandle(phandle);
#endif
  }
  clean_temp_files();
  remove(binname);
}
