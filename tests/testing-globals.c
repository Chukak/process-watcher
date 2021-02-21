#include "testing-globals.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

extern int __testing_globals_return_code;

static const char* TEXT_COLOR_RED = "\x1B[31m";
static const char* TEXT_COLOR_NORMAL = "\x1B[0m";
static const char* TEXT_COLOR_GREEN = "\x1B[32m";

__attribute__((nothrow)) static const char* check_support_color(const char* color)
{
  const char* env = getenv("TERM");
  if (env != NULL) {
    const int termsc = 7;
    const char* terms[] = {"xterm", "xterm-256", "xterm-256color", "linux", "cygwin", "color", "ansi"};
    for (int i = 0; i < termsc; ++i) {
      if (strcmp(env, terms[i]) == 0) {
        return color;
      }
    }
  }
  return "";
}

__attribute__((nothrow)) void __testing_globals_print_hdr(const char* testname, const char* testcasename)
{
  printf("%s[%s.%s] %s \n",
         check_support_color(TEXT_COLOR_GREEN),
         testname,
         testcasename,
         check_support_color(TEXT_COLOR_NORMAL));
}

__attribute__((nothrow)) void __testing_globals_print_test_result(int __failed, int __passed)
{
  if (__failed == 0) {
    printf("%s[PASSED: %d] %s \n",
           check_support_color(TEXT_COLOR_GREEN),
           __passed,
           check_support_color(TEXT_COLOR_NORMAL));
  } else {
    printf("%s[FAILED: %d, PASSED: %d] %s \n",
           check_support_color(TEXT_COLOR_RED),
           __failed,
           __passed,
           check_support_color(TEXT_COLOR_NORMAL));
  }
}

__attribute__((nothrow)) void __testing_globals_print_fail_info(const char* varname1,
                                                                const char* varname2,
                                                                const char* desc,
                                                                const char* filename,
                                                                int line,
                                                                __testing_globals_types_t __type,
                                                                int argc,
                                                                ...)
{
  printf("Check failure in %s:%d: \n\t", filename, line);

  va_list args;
  va_start(args, argc);

  switch (__type) {
  case __CHAR_PTR: {
    const char *arg1 = va_arg(args, const char*), *arg2 = va_arg(args, const char*);
    printf("%s: %s \nTo be %s\n\t%s: %s", varname1, arg1, desc, varname2, arg2);
    break;
  }
  case __VOID_PTR: {
    void *arg1 = va_arg(args, void*), *arg2 = va_arg(args, void*);
    printf("%s: %p \nTo be %s\n\t%s: %p", varname1, arg1, desc, varname2, arg2);
    break;
  }
  case __UNSIGNED_CHAR:
  case __UNSIGNED_SHORT_INT:
  case __UNSIGNED_INT: {
    unsigned arg1 = va_arg(args, unsigned), arg2 = va_arg(args, unsigned);
    printf("%s: %u \nTo be %s\n\t%s: %u", varname1, arg1, desc, varname2, arg2);
    break;
  }
  case __UNSIGNED_LONG_INT: {
    unsigned long arg1 = va_arg(args, unsigned long), arg2 = va_arg(args, unsigned long);
    printf("%s: %lu \nTo be %s\n\t%s: %lu", varname1, arg1, desc, varname2, arg2);
    break;
  }
  case __CHAR:
  case __SHORT_INT:
  case __INT: {
    int arg1 = va_arg(args, int), arg2 = va_arg(args, int);
    printf("%s: %d \nTo be %s\n\t%s: %d", varname1, arg1, desc, varname2, arg2);
    break;
  }
  case __LONG_INT: {
    long int arg1 = va_arg(args, long int), arg2 = va_arg(args, long int);
    printf("%s: %ld \nTo be %s\n\t%s: %ld", varname1, arg1, desc, varname2, arg2);
    break;
  }

  case __FLOAT:
  case __DOUBLE:
  case __LONG_DOUBLE: {
    double arg1 = va_arg(args, double), arg2 = va_arg(args, double);
    printf("%s: %f \nTo be %s\n\t%s: %f", varname1, arg1, desc, varname2, arg2);
    break;
  }
  default:
    break;
  }
  va_end(args);

  printf("\n");
}
