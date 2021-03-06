#include "process-watcher-globals.h"

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

__attribute__((nothrow)) static void strrecreate(char **dst)
{
  char *tmp = malloc(1 * sizeof(char));
  *tmp = '\0';
  *dst = tmp;
}

__attribute__((nothrow)) static int strrealloc(char **dst, int size)
{
  char *allocated = realloc(*dst, size * sizeof(char) + 1);
  if (!allocated)
    return 1;
  *dst = allocated;
  return 0;
}

__attribute__((nothrow)) void strconcat(char **dst, int count, ...)
{
  unsigned length = 0;
  char *array;
  strrecreate(&array);
  va_list args;
  va_start(args, count);
  while (count-- > 0) {
    const char *arg = va_arg(args, const char *);
    if (arg == NULL)
      break;
    length += strlen(arg);

    if (strrealloc(&array, length) != 0)
      break;
    strcat(array, arg);
  }
  va_end(args);

  *dst = array;
}

__attribute__((nothrow)) int freadall(const char *filename, char **dst)
{
  size_t bytes = 0, length = 0;
  FILE *file = fopen(filename, "r");
  if (file) {
    do {
      fseek(file, 0, SEEK_END);
      length = ftell(file);
      fseek(file, 0, SEEK_SET);

      char *array;
      strrecreate(&array);
      if (strrealloc(&array, length) != 0)
        break;

      if ((bytes = fread(array, sizeof(char), length, file)) == length) {
        array[length] = '\0';
        *dst = array;
      }
    } while (0);
    fclose(file);
  }

  return bytes;
}

__attribute__((nothrow)) void itostr(int n, char **dst)
{
  char *array;
  strrecreate(&array);
  int length = snprintf(NULL, 0, "%d", n);
  if (strrealloc(&array, length) != 0)
    return;
  ++length; // because, array has the null terminated character
  snprintf(array, length, "%d", n);

  *dst = array;
}

__attribute__((nothrow)) void strreplace(const char *src, char **dst, const char *substr, const char *repstr, int count)
{
  int sublen = strlen(substr), // substring length
      replen = strlen(repstr), // replacement length
      extrastrlen = 0;         // length string before substring

  int newstr_len = 0;
  char *newstr; // string with replacements
  strrecreate(&newstr);
  char *newstr_ptr = newstr; // pointer to the current end position in this string

  char *sub;
  for (int i = 0;                                                         //
       (sub = strstr(src, substr)) != NULL && (i < count || count == -1); // if count == -1, replace all substrings
       src += extrastrlen + sublen,                                       // increare pointer to source string
       ++i) {
    // gets all characters before substring
    extrastrlen = sub - src;

    if (strrealloc(&newstr, newstr_len + extrastrlen) != 0) // increase memory to append string before substring
      return;
    newstr_ptr = newstr + newstr_len;     // increase pointer to the end of 'newstr'
    memcpy(newstr_ptr, src, extrastrlen); // TODO: maybe use memmove?
    newstr_len += extrastrlen;            // update length

    if (strrealloc(&newstr, newstr_len + replen) != 0)
      return;
    newstr_ptr = newstr + newstr_len; // increase pointer
    memcpy(newstr_ptr, repstr, replen);
    newstr_len += replen; // update length
  }
  // copy a remaining data
  int src_remaining_len = strlen(src);
  if (strrealloc(&newstr, newstr_len + src_remaining_len) != 0)
    return;
  if (newstr_len == 0) {
    // no one substring found
    strcpy(newstr, src);
  } else {
    newstr_ptr = newstr + newstr_len;
    memcpy(newstr_ptr, src, src_remaining_len);
    newstr_len += src_remaining_len;
    newstr[newstr_len] = '\0'; // null terminated character
  }

  *dst = newstr;
}
