#ifndef __PROCESS_H
#define __PROCESS_H

/**
 * @brief pid_by_name
 * Search for the PID of the running process by his name. If this process isn't started or not found, returns -1.
 *
 * Search performed in the '/proc'/ directory.
 * @param name Process name
 * @return Return PID or -1
 */
__attribute__((nothrow)) int pid_by_name(const char* name);

#endif // __PROCESS_H
