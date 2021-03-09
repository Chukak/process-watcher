#ifndef __PROCESS_H
#define __PROCESS_H

#include <stdbool.h>

/**
 * @brief pid_by_name
 * Searches for the PID of the running process by his name. If this process isn't started or not found, returns -1.
 *
 * Search performed in the '/proc'/ directory.
 * @param name Process name
 * @return Return PID or -1
 */
__attribute__((nothrow)) int pid_by_name(const char* name);
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
  int Pid;              //! PID of the running process
  char* Process_name;   //! The process name
  char State;           //! State. (a default state = 'U' - Unknown)
  char* State_fullname; //! State as string.
  long int Priority;    //! Priority
  double Cpu_usage;     //! CPU usage
  double Memory_usage;  //! Memory usage
  char* Start_time;     //! Start time
  char* Time_usage;     //! Work time
  int Uid;              //! Uid
  char* Username;       //! User name
  // private fields
  double __last_utime;
  double __last_stime;
  double __last_total;
  unsigned long __last_starttime;
  unsigned long __last_btime;
} Process_stat;

/**
 * @brief Process_stat_init
 * Initializes the new Process_stat structure with default values.
 * @param stat The pointer to the structure
 */
__attribute__((nothrow)) void Process_stat_init(Process_stat** stat);
/**
 * @brief Process_stat_set_pid
 * Searches for the PID by the passed process name and stores this PID to the passed Process_stat structure. If PID not
 * found, stores the error message in the 'errormsg' parameter.
 * @param stat The pointer to the structure
 * @param processname Process name
 * @param errormsg Pointer to char array.
 * @return Result of searching for the PID
 */
__attribute__((nothrow)) bool Process_stat_set_pid(Process_stat** stat, const char* processname, char** errormsg);
/**
 * @brief Process_stat_update
 * Updates all public fields in the Process_stat structure. If any error occurs during the update, stores the error
 * message in the 'errormsg' parameter.
 * @param stat The pointer to the structure
 * @param errormsg
 * @return
 */
__attribute__((nothrow)) bool Process_stat_update(Process_stat** pstat, char** errormsg);
/**
 * @brief Process_stat_free
 * Deletes the Process_stat structure.
 * @param stat The pointer to the structure
 */
__attribute__((nothrow)) void Process_stat_free(Process_stat** stat);

#endif // __PROCESS_H
