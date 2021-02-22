#include "testing-globals.h"

#include "process.h"
#include "system-watcher-globals.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

TEST_CASE(Process, GetProcessNameByPid)
{
  // compile test binary
  const char *processCode = "#include <stdio.h>\n"
                            "#include <unistd.h>\n"
                            "int main(){\n"
                            "while(1){printf(\"Test message.\");sleep(1);}\n"
                            "}";
  const char *srcName = "test_process.c";
  FILE *src = fopen(srcName, "w");
  assert(src != NULL);
  assert(fprintf(src, "%s", processCode) > 0);
  assert(fclose(src) == 0);
  assert(system("gcc -o testprocess test_process.c ") == 0);
  assert(chmod(srcName, S_IRWXU | S_IRWXO) == 0);

  {
    assert(system("./testprocess > /dev/null & 2>&1 && sleep 2") == 0);

    int pid = pid_by_name("testprocess");
    CHECK_NE(pid, -1);

    // get the actual PID using pidof command
    char buf[512];
    FILE *pipe = popen("pidof testprocess", "r");
    assert(pipe != NULL);
    CHECK_NE(fgets(buf, 512, pipe), NULL);
    pclose(pipe);

    int actualPid = strtol(buf, NULL, 10);
    assert(actualPid > 0);
    // compare PID's
    CHECK_EQ(pid, actualPid);

    kill(actualPid, SIGKILL);
  }
  remove(srcName);
  remove("testprocess");
}
