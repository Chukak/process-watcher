#ifndef __TWINDOW_H
#define __TWINDOW_H

#ifdef __linux__
#include <ncurses.h>
#elif _WIN32
#include <curses.h>
#endif
#include "../include/process.h"
#include <stdbool.h>

/**
 @brief Window
 * Stores the pointer to the main window on the terminal;.
 */
typedef struct
{
  WINDOW* __p;
} Window;

/**
 * @brief Window_init
 * Initializes the new Window structure with default values.
 * @return The pointer to the structure
 */
DECLFUNC Window* Window_init() ATTR(warn_unused_result);
/**
 * @brief Window_refresh
 * Refresh the main window with data.
 * @param win The pointer to the Window structure
 * @param proc_stat The pointer to the Process_stat structure
 * @return The result of updating
 */
DECLFUNC bool Window_refresh(Window* win, Process_stat* proc_stat) ATTR(nonnull(1, 2));
/**
 * @brief Window_destroy
 * Deletes the Window structure.
 * @param win The pointer to the structure
 */
DECLFUNC void Window_destroy(Window* win) ATTR(nonnull(1));

#endif // __TWINDOW_H
