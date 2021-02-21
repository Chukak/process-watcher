#include "testing-globals.h"

#include "system-watcher-globals.h"

#include <stdio.h>

TEST_CASE(String, StringConcat)
{
  {
    char *word;
    strconcat(&word, 4, "Hello", " ", "world", "!");
    CHECK_STR_EQ(word, "Hello world!");
    free(word);
  }
  {
    char *word;
    strconcat(&word, 3, "Hey", " hello", " ", "world", "!");
    CHECK_STR_NE(word, "Hey hello world!");
    free(word);
  }
  {
    char *word;
    strconcat(&word, 5, "/proc", "/", "19256", "/", "cmdline");
    CHECK_STR_EQ(word, "/proc/19256/cmdline");
    free(word);
  }
  {
    char *word;
    // we passed only 3 strings and NULL (see macro SAFE_PASS_VARGS), because va_arg does'nt work correctly if the
    // numbers of argument greater than what was actually passed
    strconcat(&word, 7, SAFE_PASS_VARGS("/home", "/", "user"));
    CHECK_STR_NE(word, "/proc/19256/cmdline");
    free(word);
  }
  {
    char *word;
    strconcat(&word, 0, "a", "b", "c");
    CHECK_STR_EQ(word, "");
    free(word);
  }
}

TEST_CASE(String, StringReplace)
{
  {
    char *word;
    const char *source = "aaa bbb aaa ccc aaa ddd", *result = "eee bbb eee ccc eee ddd";
    strreplace(source, &word, "aaa", "eee", 3);
    CHECK_STR_EQ(word, result);
    free(word);
  }
  {
    char *word;
    const char *source = "aaa bbb aaa ccc aaa ddd", *result = "eee bbb aaa ccc aaa ddd";
    strreplace(source, &word, "aaa", "eee", 1);
    CHECK_STR_EQ(word, result);
    free(word);
  }
  {
    char *word;
    const char *source = "haha haha haha", *result = "hehe hehe hehe";
    strreplace(source, &word, "a", "e", -1); // -1 = relace all
    CHECK_STR_EQ(word, result);
    free(word);
  }
  {
    char *word;
    const char *source = "word word word word", *result = "wordwordwordword";
    strreplace(source, &word, " ", "", -1);
    CHECK_STR_EQ(word, result);
    free(word);
  }
  {
    char *word;
    const char *source = "", *result = "";
    strreplace(source, &word, "Hello", "Hi", -1);
    CHECK_STR_EQ(word, result);
    free(word);
  }
  {
    char *word;
    const char *source = "a b c d e f", *result = "a b c d e f";
    strreplace(source, &word, "z", "a", -1);
    CHECK_STR_EQ(word, result);
    free(word);
  }
}

TEST_CASE(String, IntToString)
{
  {
    char *number;
    itostr(10, &number);
    CHECK_STR_EQ(number, "10");
    free(number);
  }
  {
    char *number;
    itostr(12345678, &number);
    CHECK_STR_EQ(number, "12345678");
    free(number);
  }
  {
    char *number;
    itostr(-100, &number);
    CHECK_STR_EQ(number, "-100");
    free(number);
  }
  {
    char *number;
    itostr((int) 872346734534, &number);
    CHECK_STR_NE(number, "872346734534");
    free(number);
  }
}

TEST_CASE(File, FileReadAll)
{
  const char *testfilename = "testfile.txt";
  const char *testfiledata = "Testing data\n";

  FILE *testfile = fopen(testfilename, "w");
  assert(testfile != NULL);
  assert(fprintf(testfile, "%s", testfiledata) > 0);
  assert(fclose(testfile) == 0);
  {
    char *array;
    CHECK_GT(freadall(testfilename, &array), 0);
    CHECK_STR_EQ(array, testfiledata);
    free(array);
  }
  remove(testfilename);
}
