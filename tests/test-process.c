#include "testing-globals.h"

#include "process.h"
#include "process-watcher-globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

__attribute__((nothrow)) static int get_real_pid(const char *name)
{
  int pid = -1;
  char buf[512], command[512];
  sprintf(command, "pidof %s", name);
  FILE *pipe = popen(command, "r");
  assert(pipe != NULL);
  if (fgets(buf, 512, pipe) != NULL) {
    pclose(pipe);
    pid = strtol(buf, NULL, 10);
  }
  return pid;
}

__attribute__((nothrow)) static void compile_binary(const char *name, const char *code)
{
  const char *srcName = "src.c";
  FILE *src = fopen(srcName, "w");
  assert(src != NULL);
  assert(fprintf(src, "%s", code) > 0);
  assert(fclose(src) == 0);

  char command[512];
  sprintf(command, "gcc -o %s src.c", name);
  assert(system(command) == 0);
  assert(chmod(srcName, S_IRWXU | S_IRWXO) == 0);
}

__attribute__((nothrow)) static void clean_temp_files()
{
  remove("src.c");
}

TEST_CASE(Process, GetProcessNameByPid)
{
  // compile test binary
  const char *processCode = "#include <stdio.h>\n"
                            "#include <unistd.h>\n"
                            "int main(){\n"
                            "while(1){printf(\"Test message.\");sleep(1);}\n"
                            "}";
  compile_binary("testprocess", processCode);
  {
    assert(system("./testprocess > /dev/null & 2>&1 && sleep 2") == 0);

    int pid = pid_by_name("testprocess");
    CHECK_NE(pid, -1);

    // get the actual PID using pidof command
    int actualPid = get_real_pid("testprocess");
    assert(actualPid > 0);
    // compare PID's
    CHECK_EQ(pid, actualPid);

    kill(actualPid, SIGKILL);
  }
  clean_temp_files();
  remove("testprocess");
}

TEST_CASE(Process, ProcessStatStructureUsage)
{
  // compile test binary
  const char *processCode = "#include <stdio.h>\n"
                            "#include <unistd.h>\n"
                            "#include <stdlib.h>\n"
                            "int main(){\n"
                            "char *buffer = malloc(8128 * sizeof(char));\n"
                            "while(1){\n"
                            "printf(\"Test message.\");sleep(1);\n"
                            "for(long i=0; i < 180000000000; ++i);\n"
                            "}\n"
                            "free(buffer);"
                            "}\n";
  compile_binary("testprocess", processCode);
  {
    assert(system("./testprocess > /dev/null & 2>&1 && sleep 2") == 0);
    int actualPid = get_real_pid("testprocess");
    assert(actualPid > 0);

    Process_stat *statobj = NULL;
    Process_stat_init(&statobj);
    CHECK_NE(statobj, NULL);

    char *errormsg = NULL;
    CHECK_EQ(Process_stat_set_pid(&statobj, "testprocess", &errormsg), true);
    CHECK_EQ(statobj->Pid, actualPid);
    CHECK_STR_EQ(statobj->Process_name, "testprocess");
    CHECK_EQ(statobj->State, 'U'); // Unknown state
    CHECK_STR_EQ(statobj->State_fullname, "Unknown");
    CHECK_STR_EQ(statobj->Time_usage, "00:00:00");
    CHECK_STR_EQ(statobj->Start_time, "00:00:00");
    CHECK_EQ(statobj->Uid, -1);
    CHECK_EQ(statobj->Username, NULL);
    CHECK_EQ(statobj->Killed, false);

    CHECK_EQ(Process_stat_update(&statobj, &errormsg), true);
    CHECK_EQ(statobj->Pid, actualPid);

    SLEEP_SEC(8);

    CHECK_EQ(Process_stat_update(&statobj, &errormsg), true);
    CHECK_EQ(statobj->Pid, actualPid);
    CHECK_EQ(statobj->State, 'R'); // Running state
    CHECK_GT(statobj->Cpu_usage, 0.0);
    CHECK_GT(statobj->Memory_usage, 0.0);
    CHECK_EQ(statobj->Cpu_peak_usage, statobj->Cpu_usage);
    CHECK_EQ(statobj->Memory_peak_usage, statobj->Memory_usage);
    CHECK_STR_EQ(statobj->State_fullname, "Running");
    CHECK_STR_NE(statobj->Time_usage, "00:00:00");
    CHECK_STR_NE(statobj->Start_time, "00:00:00");
    CHECK_NE(statobj->Uid, -1);
    CHECK_NE(statobj->Username, NULL);
    CHECK_EQ(statobj->Killed, false);

    CHECK_EQ(Process_stat_kill(&statobj, &errormsg), true);   // kill
    CHECK_EQ(Process_stat_update(&statobj, &errormsg), true); // refresh
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
    CHECK_NE(statobj->Uid, -1);
    CHECK_NE(statobj->Username, NULL);
    CHECK_EQ(statobj->Killed, true);

    SLEEP_SEC(2);
    CHECK_LT(getpgid(actualPid), 0);

    Process_stat_free(&statobj);
    CHECK_EQ(statobj, NULL);

    {
      if (getpgid(actualPid) >= 0)
        kill(actualPid, SIGKILL);
    }
  }
  clean_temp_files();
  remove("testprocess");
}
