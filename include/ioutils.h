#ifndef __IOUTILS_H
#define __IOUTILS_H

#include "props.h"

/**
 * @brief SAFE_PASS_VARGS
 * Macro for use with variadic parameters. Macro adds a NULL pointer at the end.
 */
#define SAFE_PASS_VARGS(...) __VA_ARGS__, NULL

/**
 * @brief strconcat
 * Cancatenates number of strings.
 * For passing string va_args macro is used. This function dynamically allocates a char array for the result of
 * concatenation. Recomended to use with SAFE_PASS_VARGS (or pass NULL as the last parameter) and unitialize char array
 * passed the first parameter.
 *
 * If number of strings greater the strings actually passed, this function has an undefined behavior.
 * If dynamic allocate memory is not possible, this function stores the last concatenated strings and returns.
 *
 * @code
 * char* array = NULL;
 * strconcat(&array, 5, SAFE_PASS_VARGS("a", "b", "c", "d", "e"));
 * // use array variable
 * free(array);
 * @endcode
 *
 * @param dst The array to store result
 * @param count Number of strings for concatenation
 */
EXTERNFUNC DECLFUNC void strconcat(char** dst, int count, ...);
/**
 * @brief freadall
 * Reads all data from file. This functions dynamically allocates a char array for the data
 * from the file. This function uses a precalculation of the file length before reading the data.
 *
 * If dynamic allocate memory is not possible, this function do nothing.
 *
 * @code
 * char *dst = NULL;
 * freadall("filename.txt", &dst);
 * // ...
 * free(dst);
 * @endcode
 *
 * @param filename The name of file
 * @param dst The array to store data
 * @return Number of bytes read
 */
EXTERNFUNC DECLFUNC long long freadall(const char* filename, char** dst);
/**
 * @brief itostr
 * Convert an integer number to string. This function dynamically allocates a char array for the store integer.
 *
 * If dynamic allocate memory is not possible, this function do nothing.
 *
 * @code
 * char *dst = NULL;
 * itostr(100, &dst);
 * // ...
 * free(dst);
 * @endcode
 *
 * @param n The number
 * @param dst The array to store number
 */
EXTERNFUNC DECLFUNC void itostr(int n, char** dst);
/**
 * @brief strreplace
 * Replaces substrings in the passed string. The specified number of substrings wil be replaced, if all substrings are
 * found. If number of substrings to replace is greater than all actual substrings, then all substrings will be
 * replaced. The same is true, if number of substring is less than zero.
 *
 * @code
 * char *dst = NULL;
 * streplace("aaa bbb aaa", &dst, "a", "e", -1);
 * // ...
 * free(dst);
 * @endcode
 *
 * @param src The source string
 * @param dst The array to store replaced string
 * @param substr The substring to replace
 * @param repstr The replacement string
 * @param count Number of substring to replace
 */
EXTERNFUNC DECLFUNC void strreplace(const char* src, char** dst, const char* substr, const char* repstr, int count)
    ATTR(nonnull(1, 3, 4));
/**
 * @brief fgetall
 * Reads all data from file. This functions dynamically allocates a char array for the data
 * from the file. This function read data until EOF or error occurs.
 *
 * If dynamic allocate memory is not possible, this function do nothing.
 *
 * @code
 * char *dst = NULL;
 * fgetall("filename.txt", &dst);
 * // ...
 * free(dst);
 * @endcode
 *
 * @param filename The name of file
 * @param dst The array to store data
 * @return Number of bytes read or -1 if the file was not opened
 */
EXTERNFUNC DECLFUNC long long fgetall(const char* filename, char** dst);
/**
 * @brief ftostr
 * Convert a float number to string. This function dynamically allocates a char array for the store integer.
 *
 * If dynamic allocate memory is not possible, this function do nothing.
 *
 * @code
 * char *dst = NULL;
 * ftostr(0.77, &dst);
 * // ...
 * free(dst);
 * @endcode
 * @param n The number
 * @param dst The array to store number
 */
EXTERNFUNC DECLFUNC void ftostr(double n, char** dst);
/**
 * @brief ulltostr
 * Convert an unsigned long long number to string. This function dynamically allocates a char array for the store
 * integer.
 *
 * If dynamic allocate memory is not possible, this function do nothing.
 *
 * @code
 * char *dst = NULL;
 * ulltostr(12345678, &dst);
 * // ...
 * free(dst);
 * @endcode
 * @param n The number
 * @param dst The array to store number
 */
EXTERNFUNC DECLFUNC void ulltostr(unsigned long long n, char** dst);
#endif // __IOUTILS_H
