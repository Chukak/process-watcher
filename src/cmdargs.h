#ifndef __CMDARGS_H
#define __CMDARGS_H

#include "props.h"
#include <stdbool.h>

/**
 @brief Cmd_args
 * Stores arguments from command line. Contains the process name, error message (if an error occurred), the timeout to
 refresh the process information.
 */
typedef struct
{
  bool Valid;
  char* Process_name;
  long int Refresh_timeout_ms;
  char* Errormsg;
} Cmd_args;

/**
 * @brief Cmd_args_init
 * Parses command line arguments and initializes the cmd_args structure.
 * @param argc Numbers of arguments
 * @param argv Array with arguments
 * @return The pointer to the structure
 */
DECLFUNC Cmd_args* Cmd_args_init(int argc, char** argv) ATTR(warn_unused_result);
/**
 * @brief Cmd_args_free
 * Deletes the Cmd_args structure.
 * @param args The pointer to the structure
 */
DECLFUNC void Cmd_args_free(Cmd_args* args) ATTR(nonnull(1));
/**
 * @brief print_help
 * Prints the help information.
 */
DECLFUNC void print_help();

#endif // __CMDARGS_H
