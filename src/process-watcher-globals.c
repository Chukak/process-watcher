#include "process-watcher-globals.h"

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const int SMALL_BUFFER_SIZE = 128;

DECLFUNC static void strrecreate(char **dst)
{
  char *tmp = malloc(1 * sizeof(char));
  *tmp = '\0';
  *dst = tmp;
}

DECLFUNC static int strrealloc(char **dst, long unsigned int size)
{
  char *allocated = realloc(*dst, size * sizeof(char) + 1);
  if (!allocated)
    return 1;
  *dst = allocated;
  return 0;
}

DECLFUNC void strconcat(char **dst, int count, ...)
{
  size_t length = 0;
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

DECLFUNC long long freadall(const char *filename, char **dst)
{
  long long bytes = 0, length = 0;
  FILE *file = fopen(filename, "r");
  if (file) {
    do {
      fseek(file, 0, SEEK_END);
      length = ftell(file);
      fseek(file, 0, SEEK_SET);

      char *array;
      strrecreate(&array);
      if (strrealloc(&array, (size_t) length) != 0)
        break;

      if ((bytes = (long long) fread(array, sizeof(char), (size_t) length, file)) == length) {
        array[length] = '\0';
        *dst = array;
      }
    } while (0);
    fclose(file);
  }

  return bytes;
}

DECLFUNC ATTR(
    nonnull(1, 3, 4)) void strreplace(const char *src, char **dst, const char *substr, const char *repstr, int count)
{
  size_t sublen = strlen(substr), // substring length
      replen = strlen(repstr),    // replacement length
      extrastrlen = 0;            // length string before substring

  size_t newstr_len = 0;
  char *newstr; // string with replacements
  strrecreate(&newstr);
  char *newstr_ptr = newstr; // pointer to the current end position in this string

  char *sub;
  for (int i = 0;                                                         //
       (sub = strstr(src, substr)) != NULL && (i < count || count == -1); // if count == -1, replace all substrings
       src += extrastrlen + sublen,                                       // increare pointer to source string
       ++i) {
    // gets all characters before substring
    extrastrlen = (size_t)(sub - src);

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
  size_t src_remaining_len = strlen(src);
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

DECLFUNC long long fgetall(const char *filename, char **dst)
{
  long long bytes = 0;
  FILE *file = fopen(filename, "r");
  if (file) {
    char *array = NULL;
    char buffer[SMALL_BUFFER_SIZE];
    while (fgets(buffer, (int) sizeof(buffer), file)) {
      size_t length = strlen(buffer);
      if (strrealloc(&array, (size_t) bytes + length) != 0)
        break;
      memcpy(array + bytes, buffer, length);
      bytes += (long long) length;
    }

    if (array) {
      array[bytes] = '\0';
      *dst = array;
    }
    fclose(file);
  } else
    bytes = -1;

  return bytes;
}

DECLFUNC static void tostr(void *p, char **dst, int typeid)
{
  char *array;
  strrecreate(&array);
  size_t length;
  switch (typeid) {
  case 0:
    length = (size_t) snprintf(NULL, 0, "%d", *((int *) p));
    break;
  case 1:
    length = (size_t) snprintf(NULL, 0, "%.3f", *((double *) p));
    break;
  default:
    break;
  }

  if (strrealloc(&array, length) != 0)
    return;
  ++length; // because, array has the null terminated character

  switch (typeid) {
  case 0:
    snprintf(array, length, "%d", *((int *) p));
    break;
  case 1:
    snprintf(array, length, "%.3f", *((double *) p));
    break;
  default:
    break;
  }

  *dst = array;
}

#define __GET_TYPEID(x) _Generic((x), int : 0, double : 1, default : -1)
#define TO_STR(n, dst) tostr(&n, dst, __GET_TYPEID(n))

DECLFUNC void itostr(int n, char **dst)
{
  TO_STR(n, dst);
}

DECLFUNC void ftostr(double n, char **dst)
{
  TO_STR(n, dst);
}
