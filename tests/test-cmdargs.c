#include "testing-globals.h"

#include "cmdargs.h"

TEST_CASE(Cmd_args, CorrectCommandLineArguments)
{
  {
    int argc = 2;
    char *argv[] = {(char *) ".", (char *) "test-process-name"};

    Cmd_args *args = Cmd_args_init(argc, argv);
    assert(args != NULL);
    CHECK_STR_EQ(args->Process_name, "test-process-name");
    CHECK_EQ(args->Refresh_timeout_ms, 1000);
    CHECK_EQ(args->Valid, true);
    CHECK_EQ(args->Errormsg, NULL);

    Cmd_args_free(args);
  }
  {
    int argc = 4;
    char *argv[] = {(char *) ".", (char *) "-refresh-timeout-ms", (char *) "500", (char *) "test-process-name"};

    Cmd_args *args = Cmd_args_init(argc, argv);
    assert(args != NULL);
    CHECK_STR_EQ(args->Process_name, "test-process-name");
    CHECK_EQ(args->Refresh_timeout_ms, 500);
    CHECK_EQ(args->Valid, true);
    CHECK_EQ(args->Errormsg, NULL);

    Cmd_args_free(args);
  }
}

TEST_CASE(Cmd_args, IncorrectCommandLineArguments)
{
  {
    int argc = 3;
    char *argv[] = {(char *) ".", (char *) "-refresh-timeout-ms", (char *) "test-process-name"};

    Cmd_args *args = Cmd_args_init(argc, argv);
    assert(args != NULL);
    CHECK_EQ(args->Valid, false);
    CHECK_STR_NE(args->Errormsg, ""); // not empty
    CHECK_EQ(args->Process_name, NULL);
    CHECK_EQ(args->Refresh_timeout_ms, -1);

    Cmd_args_free(args);
  }
  {
    int argc = 3;
    char *argv[] = {(char *) ".", (char *) "-refresh-timeout-ms", (char *) "2000"};

    Cmd_args *args = Cmd_args_init(argc, argv);
    assert(args != NULL);
    CHECK_EQ(args->Valid, false);
    CHECK_STR_NE(args->Errormsg, ""); // not empty
    CHECK_EQ(args->Process_name, NULL);
    // timeout is valid, but process name not specified
    CHECK_EQ(args->Refresh_timeout_ms, 2000);

    Cmd_args_free(args);
  }
  {
    int argc = 1;
    char *argv[] = {(char *) "."};

    Cmd_args *args = Cmd_args_init(argc, argv);
    assert(args != NULL);
    CHECK_EQ(args->Valid, false);
    CHECK_EQ(args->Errormsg, NULL);
    CHECK_EQ(args->Process_name, NULL);
    CHECK_EQ(args->Refresh_timeout_ms, -1);

    Cmd_args_free(args);
  }
}
