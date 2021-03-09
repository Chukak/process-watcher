#include "window.h"
#include "process-watcher-globals.h"

#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static const int DEFAULT_PAIR = 1; // default color pair id
static const int HEADER_PAIR = 2;  // line color pair id
static const int LOW_CPU_USAGE = 3;
static const int MEDIUM_CPU_USAGE = 4;
static const int HARD_CPU_USAGE = 5;

static const int MAX_CPU_VALUE_LENGTH =
    7; // max CPU value if XXX.XXX (example 100.121), 3 digits + dot + 3 digits as precision

__attribute__((nothrow)) Window *Window_init()
{
  Window *win = malloc(sizeof(Window));

  win->__p = initscr(); // init ncurses WINDOW

  clear();
  curs_set(0);
  refresh();

  start_color();

  //        {    id    }  {foreground} {background}
  init_pair(DEFAULT_PAIR, COLOR_WHITE, COLOR_BLACK);
  init_pair(HEADER_PAIR, COLOR_BLACK, COLOR_WHITE);
  init_pair(LOW_CPU_USAGE, COLOR_GREEN, COLOR_BLACK);
  init_pair(MEDIUM_CPU_USAGE, COLOR_YELLOW, COLOR_BLACK);
  init_pair(HARD_CPU_USAGE, COLOR_RED, COLOR_BLACK);

  return win;
}

__attribute__((nothrow)) static void draw_CPU_usage(Window *win, Process_stat *proc_stat, int termX, int termY)
{
  UNUSED(termY);

  int cursX = 0,    // cursor X position
      cursY = 0,    // cursor Y position
      roffsetX = 3; // right offset X position
  char *hdrcpu = NULL;
  {
    // cpu value
    char *cpu_str = NULL; // cpu usage as str
    char *hdrcpuoffset = NULL;
    ftostr(proc_stat->Cpu_usage, &cpu_str);
    strconcat(&hdrcpu, 3, SAFE_PASS_VARGS("CPU: ", cpu_str, "% "));

    attron(COLOR_PAIR(HEADER_PAIR));
    mvwaddstr(win->__p, cursY, cursX, hdrcpu);
    attroff(COLOR_PAIR(HEADER_PAIR));
    cursX += strlen(hdrcpu);

    {
      // whitespaces
      int spcount = MAX_CPU_VALUE_LENGTH - strlen(cpu_str);
      char spaces[spcount + 1];
      spaces[0] = '\0';

      while (spcount-- > 1)
        strcat(spaces, " ");

      strconcat(&hdrcpuoffset, 2, SAFE_PASS_VARGS(spaces, "["));
    }

    attron(COLOR_PAIR(DEFAULT_PAIR));
    mvwaddstr(win->__p, cursY, cursX, hdrcpuoffset);
    attroff(COLOR_PAIR(DEFAULT_PAIR));
    cursX += strlen(hdrcpuoffset);

    free(cpu_str);
    free(hdrcpuoffset);
  }
  {
    // print cpu usage bar
    int len_CPUbar = termX - (cursX + roffsetX);
    int len_fillbar = len_CPUbar * (proc_stat->Cpu_usage / 100);
    for (int j = 0; j < len_CPUbar; ++j) {
      if (j > len_fillbar)
        break;

      int idpair = 0;
      if (len_CPUbar - j > (len_CPUbar / 3) * 2)
        idpair = LOW_CPU_USAGE;
      else if (len_CPUbar - j > (len_CPUbar / 3))
        idpair = MEDIUM_CPU_USAGE;
      else
        idpair = HARD_CPU_USAGE;

      attron(COLOR_PAIR(idpair));
      mvwaddch(win->__p, cursY, cursX, '#');
      attroff(COLOR_PAIR(idpair));

      ++cursX;
    }
  }

  attron(COLOR_PAIR(DEFAULT_PAIR));
  mvwaddstr(win->__p, cursY, termX - roffsetX, "]   ");
  attroff(COLOR_PAIR(DEFAULT_PAIR));

  free(hdrcpu);
}

__attribute__((nothrow)) static void draw_process_info(Window *win, Process_stat *proc_stat, int termX, int termY)
{
  UNUSED(termY);
  UNUSED(termX);

  int cursY = 2,    // cursor Y position
      loffsetX = 4; // left offset X position
  {
    char *hdr = NULL; // header

    char *strPID = NULL; // pid as str

    attron(COLOR_PAIR(DEFAULT_PAIR));

    strconcat(&hdr, 3, SAFE_PASS_VARGS("Name: ", proc_stat->Process_name, " "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    itostr(proc_stat->Pid, &strPID);
    strconcat(&hdr, 3, SAFE_PASS_VARGS("PID: ", strPID, " "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    free(hdr);

    attroff(COLOR_PAIR(DEFAULT_PAIR));
    free(strPID);
  }
  cursY += 2;
  {
    char *hdr = NULL; // header

    char *strpriority = NULL, // process priority as str
        *strmemory = NULL,    // memory usage as str
            *struid = NULL;   // user id

    char strstate[2] = "\0"; // process state as str
    attron(COLOR_PAIR(DEFAULT_PAIR));

    itostr(proc_stat->Priority, &strpriority);
    strconcat(&hdr, 3, SAFE_PASS_VARGS("Priority: ", strpriority, " "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    strstate[0] = proc_stat->State;
    strconcat(&hdr, 5, SAFE_PASS_VARGS("State: '", strstate, "' (", proc_stat->State_fullname, ") "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    ftostr(proc_stat->Memory_usage, &strmemory);
    strconcat(&hdr, 3, SAFE_PASS_VARGS("Memory: ", strmemory, "MB "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    strconcat(&hdr, 3, SAFE_PASS_VARGS("Time: ", proc_stat->Time_usage, " "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    strconcat(&hdr, 3, SAFE_PASS_VARGS("Start time: ", proc_stat->Start_time, " "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    itostr(proc_stat->Uid, &struid);
    strconcat(&hdr, 5, SAFE_PASS_VARGS("User: ", proc_stat->Username, " (Uid=", struid, ") "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    free(hdr);

    attroff(COLOR_PAIR(DEFAULT_PAIR));
    free(strpriority);
    free(strmemory);
    free(struid);
  }
  cursY += 2;
  {
    char *hdr = NULL; // header

    char *strcpu = NULL, *strmemory = NULL;

    attron(COLOR_PAIR(DEFAULT_PAIR));

    ftostr(proc_stat->Cpu_peak_usage, &strcpu);
    strconcat(&hdr, 3, SAFE_PASS_VARGS("CPU peak: ", strcpu, "% "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    ftostr(proc_stat->Memory_peak_usage, &strmemory);
    strconcat(&hdr, 3, SAFE_PASS_VARGS("Memory peak: ", strmemory, "MB "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    attroff(COLOR_PAIR(DEFAULT_PAIR));
    free(strcpu);
    free(strmemory);
  }
}

__attribute__((nothrow)) void Window_refresh(Window *win, Process_stat *proc_stat)
{
  int x = 0, y = 0;
  {
    struct winsize sz; // get terminal real size at the moment
    if (ioctl(2, TIOCGWINSZ, &sz) == 0) {
      x = sz.ws_col;
      y = sz.ws_row;
    } else {
      // use defaults from ncurses
      x = COLS;
      y = LINES;
    }
  }
  wresize(win->__p, y, x);
  refresh();
  clear();

  char *errormsg = NULL;
  if (!Process_stat_update(&proc_stat, &errormsg)) {
    printw("%s\n", errormsg);
    free(errormsg);
    return;
  }

  draw_CPU_usage(win, proc_stat, x, y);
  draw_process_info(win, proc_stat, x, y);
}

__attribute__((nothrow)) void Window_destroy(Window *win)
{
  endwin(); // remove ncurses WINDOW

  free(win);
}
