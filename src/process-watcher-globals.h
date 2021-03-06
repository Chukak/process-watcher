#ifndef __PROCESS_WATCHER_GLOBALS_H
#define __PROCESS_WATCHER_GLOBALS_H

/**
 * @brief UNUSED
 * Macro makes the variable unused.
 */
#define UNUSED(__var) (void) __var;
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
__attribute__((nothrow)) void strconcat(char** dst, int count, ...);
/**
 * @brief freadall
 * Reads all data from file. This functions dynamically allocates a char array for the data
 * from the file.
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
__attribute__((nothrow)) int freadall(const char* filename, char** dst);
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
__attribute__((nothrow)) void itostr(int n, char** dst);
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
__attribute__((nothrow)) void
strreplace(const char* src, char** dst, const char* substr, const char* repstr, int count);

#endif // __PROCESS_WATCHER_GLOBALS_H
