#ifndef CMDARGS_H
#define CMDARGS_H

#include <stdbool.h>

/**
 @brief Cmd_args
 * Stores arguments from command line. Contains the process name, error message (if and error occurred), the timeout to
 refresh the process information.
 */
typedef struct
{
  bool Valid;
  char* Process_name;
  int Refresh_timeout_ms;
  char* Errormsg;
} Cmd_args;

/**
 * @brief Cmd_args_init
 * Parses command line arguments and initializes the cmd_args structure.
 * @param argc Numbers of arguments
 * @param argv Array with arguments
 * @return The pointer to the structure
 */
__attribute__((nothrow)) Cmd_args* Cmd_args_init(int argc, char** argv);
/**
 * @brief Cmd_args_free
 * Deletes the Cmd_args structure.
 * @param args The pointer to the structure
 */
__attribute__((nothrow)) void Cmd_args_free(Cmd_args* args);
/**
 * @brief print_help
 * Prints the help information.
 */
__attribute__((nothrow)) void print_help();

#endif // CMDARGS_H
