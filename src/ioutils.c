#include "ioutils.h"

#ifdef _WIN32
#include <io.h>
#elif __linux__
#include <unistd.h>
#endif

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SMALL_BUFFER_SIZE 128

DECLFUNC static void strrecreate(char **dst)
{
  char *tmp = malloc(1 * sizeof(char));
  ASSERT(tmp != NULL, "unable to create string (char*) != NULL; malloc(...) returns NULL.");
  *tmp = '\0';
  *dst = tmp;
}

static int strrealloc(char **dst, size_t size)
{
  char *allocated = realloc(*dst, size * sizeof(char) + 1);
  if (!allocated)
    return 1;
  *dst = allocated;
  return 0;
}

void strconcat(char **dst, int count, ...)
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

long long freadall(const char *filename, char **dst)
{
  long long bytes = 0, length = 0;
  FILE *file = fopen(filename, "r");
  if (file) {
    do {
      fseek(file, 0, SEEK_END);
      length = ftell(file);
      fseek(file, 0, SEEK_SET);

      if (length <= 0)
        break;

      char *array;
      strrecreate(&array);
      if (strrealloc(&array, (size_t) length) != 0)
        break;

      if ((bytes = (long long) fread(array,
                                     sizeof(char),
                                     (size_t) length,
                                     file)) /* less or equal, because we can read bytes less than length */
          <= length) {
        array[bytes] = '\0';
        *dst = array;
      }

    } while (0);
    fclose(file);
  }

  return bytes;
}

void strreplace(const char *src, char **dst, const char *substr, const char *repstr, int count)
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

long long fgetall(const char *filename, char **dst)
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

typedef enum
{
  INT_T,
  DOUBLE_T,
  UNSIGLED_LONG_LONG_T
} __type_id;

static void tostr(void *p, char **dst, __type_id typeid)
{
  char *array;
  strrecreate(&array);
  long long int length;
  switch (typeid) {
  case INT_T:
    length = snprintf(NULL, 0, "%d", *((int *) p));
    break;
  case DOUBLE_T:
    length = snprintf(NULL, 0, "%.3f", *((double *) p));
    break;
  case UNSIGLED_LONG_LONG_T:
    length = snprintf(NULL, 0, "%llu", *((unsigned long long *) p));
    break;
  default:
    return;
  }

  if (length < 0)
    return;

  if (strrealloc(&array, (size_t) length) != 0)
    return;
  ++length; // because, array has the null terminated character

  switch (typeid) {
  case INT_T:
    snprintf(array, (size_t) length, "%d", *((int *) p));
    break;
  case DOUBLE_T:
    snprintf(array, (size_t) length, "%.3f", *((double *) p));
    break;
  case UNSIGLED_LONG_LONG_T:
    snprintf(array, (size_t) length, "%llu", *((unsigned long long *) p));
    break;
  default:
    break;
  }

  *dst = array;
}

void itostr(int n, char **dst)
{
  tostr(&n, dst, INT_T);
}

void ftostr(double n, char **dst)
{
  tostr(&n, dst, DOUBLE_T);
}

void ulltostr(unsigned long long n, char **dst)
{
  tostr(&n, dst, UNSIGLED_LONG_LONG_T);
}
