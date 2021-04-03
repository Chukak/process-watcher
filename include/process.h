#ifndef __PROCESS_H
#define __PROCESS_H

#include "props.h"
#include <stdbool.h>

/**
 * @brief pid_by_name
 * Searches for the PID of the running process by his name. If this process isn't started or not found, returns -1.
 *
 * Search performed in the '/proc'/ directory.
 * @param name Process name
 * @return Return PID or -1
 */
EXTERNFUNC DECLFUNC int pid_by_name(const char* name);
/**
 * @brief Process_stat
 * Stores the information about the running process from '/proc/[pid]' directory. Contains PID, the process name, state,
 * priority, user, CPU, memory and time usage.
 * Also, this structure contains private fields with the '__' prefix. Do not use it.
 *
 * For more informations, check this page https://man7.org/linux/man-pages/man5/proc.5.html
 */
typedef struct
{
  int Pid;                  //! PID of the running process
  char* Process_name;       //! The process name
  char State;               //! State. (a default state = 'U' - Unknown)
  char* State_fullname;     //! State as string.
  int Priority;             //! Priority
  double Cpu_usage;         //! CPU usage
  double Cpu_peak_usage;    //! CPU peak usage
  double Memory_usage;      //! Memory usage
  double Memory_peak_usage; //! Memory peak usage
  char* Start_time;         //! Start time
  char* Time_usage;         //! Work time
#ifdef __linux__
  int Uid; //! Uid
#endif
  char* Username; //! User name
  bool Killed;
  // private fields
  unsigned long long __last_utime;
  unsigned long long __last_stime;
  unsigned long long __last_total;
  unsigned long long __last_starttime;
#ifdef __linux__
  unsigned long long __last_btime;
#endif
#ifdef _WIN32
  void* __phandle;
#endif
} Process_stat;

/**
 * @brief Process_stat_init
 * Initializes the new Process_stat structure with default values.
 * @return The pointer to the new structure
 */
EXTERNFUNC DECLFUNC Process_stat* Process_stat_init() ATTR(warn_unused_result);
/**
 * @brief Process_stat_set_pid
 * Searches for the PID by the passed process name and stores this PID to the passed Process_stat structure. If PID not
 * found, stores the error message in the 'errormsg' parameter.
 * @param stat The pointer to the structure
 * @param processname Process name
 * @param errormsg Pointer to char array.
 * @return Result of searching for the PID
 */
EXTERNFUNC DECLFUNC bool Process_stat_set_pid(Process_stat* stat, const char* processname, char** errormsg)
    ATTR(nonnull(1, 2));
/**
 * @brief Process_stat_update
 * Updates all public fields in the Process_stat structure. If any error occurs during the update, stores the error
 * message in the 'errormsg' parameter.
 * @param stat The pointer to the structure
 * @param errormsg Pointer to char array.
 * @return Result of updating.
 */
EXTERNFUNC DECLFUNC bool Process_stat_update(Process_stat* pstat, char** errormsg) ATTR(nonnull(1));
/**
 * @brief Process_stat_kill
 * Kills the current process by PID. If any error occurs during the destruction process, stores the error
 * message in the 'errormsg' parameter.
 * @param stat The pointer to the structure
 * @param errormsg Pointer to char array.
 * @return Result of destruction.
 */
EXTERNFUNC DECLFUNC bool Process_stat_kill(Process_stat* stat, char** errormsg) ATTR(nonnull(1));
/**
 * @brief Process_stat_free
 * Deletes the Process_stat structure.
 * @param stat The pointer to the structure
 */
EXTERNFUNC DECLFUNC void Process_stat_free(Process_stat* stat) ATTR(nonnull(1));

#endif // __PROCESS_H
