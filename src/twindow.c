#include "twindow.h"
#include "ioutils.h"

#include <string.h>
#include <stdlib.h>
#ifdef __linux__
#include <sys/ioctl.h>
#include <unistd.h>
#elif _WIN32
#include <Windows.h>
#endif

static const short int DEFAULT_PAIR = 1; // default color pair id
static const short int HEADER_PAIR = 2;  // line color pair id
static const short int LOW_CPU_USAGE = 3;
static const short int MEDIUM_CPU_USAGE = 4;
static const short int HARD_CPU_USAGE = 5;
static const short int MENU_PAIR = 6;

static const int MAX_CPU_VALUE_LENGTH =
    7; // max CPU value if XXX.XXX (example 100.121), 3 digits + dot + 3 digits as precision

Window *Window_init()
{
  Window *win = malloc(sizeof(Window));
  ASSERT(win != NULL, "win (Window*) != NULL; malloc(...) returns NULL.");
  win->__p = initscr(); // init ncurses WINDOW

  clear();
  curs_set(0);
  noecho();
  cbreak();
  nodelay(win->__p, true); // non-blocking getch
  wrefresh(win->__p);

  if (has_colors()) // TODO: if no colors?
    start_color();

  //        {    id    }  {foreground} {background}
  init_pair(DEFAULT_PAIR, COLOR_WHITE, COLOR_BLACK);
  init_pair(HEADER_PAIR, COLOR_BLACK, COLOR_WHITE);
  init_pair(LOW_CPU_USAGE, COLOR_GREEN, COLOR_BLACK);
  init_pair(MEDIUM_CPU_USAGE, COLOR_YELLOW, COLOR_BLACK);
  init_pair(HARD_CPU_USAGE, COLOR_RED, COLOR_BLACK);
#ifdef __linux__
  init_pair(MENU_PAIR, COLOR_BLACK, COLOR_MAGENTA);
#elif _WIN32
  init_pair(MENU_PAIR, COLOR_WHITE, COLOR_BLACK);
#endif

  return win;
}

static void draw_CPU_usage(Window *win, Process_stat *proc_stat, int termX, int termY)
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
    cursX += (int) strlen(hdrcpu);

    {
      // whitespaces
      int spcount = MAX_CPU_VALUE_LENGTH - (int) strlen(cpu_str);
      char *spaces = malloc(sizeof(char) * (size_t) spcount + 1);
      ASSERT(spaces != NULL, "spaces (char*) != NULL; malloc(...) returns NULL.");
      spaces[0] = '\0';

      while (spcount-- > 1)
        strcat(spaces, " ");

      strconcat(&hdrcpuoffset, 2, SAFE_PASS_VARGS(spaces, "["));
      free(spaces);
    }
    attron(COLOR_PAIR(DEFAULT_PAIR));
    mvwaddstr(win->__p, cursY, cursX, hdrcpuoffset);
    attroff(COLOR_PAIR(DEFAULT_PAIR));
    cursX += (int) strlen(hdrcpuoffset);

    free(cpu_str);
    free(hdrcpuoffset);
  }
  {
    // print cpu usage bar
    int len_CPUbar = termX - (cursX + roffsetX);
    int len_fillbar = len_CPUbar * (int) (proc_stat->Cpu_usage / 100);
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

static void draw_process_info(Window *win, Process_stat *proc_stat, int termX, int termY)
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

#ifdef _WIN32
    strconcat(&hdr, 3, SAFE_PASS_VARGS("User: ", proc_stat->Username, " "));
#elif __linux__
    itostr(proc_stat->Uid, &struid);
    strconcat(&hdr, 5, SAFE_PASS_VARGS("User: ", proc_stat->Username, " (Uid=", struid, ") "));
#endif
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
    free(hdr);

    attroff(COLOR_PAIR(DEFAULT_PAIR));
    free(strcpu);
    free(strmemory);
  }
  cursY += 2;
  {
    char *hdr = NULL; // header

    char *strdisk_read = NULL, *strdisk_written = NULL, *strdisk_read_usage = NULL, *strdisk_write_usage,
         *strdisk_read_peak_usage = NULL, *strdisk_write_peak_usage = NULL;

    attron(COLOR_PAIR(DEFAULT_PAIR));

    ftostr(proc_stat->Disk_read_mb_usage, &strdisk_read_usage);
    ftostr(proc_stat->Disk_write_mb_usage, &strdisk_write_usage);
    strconcat(&hdr, 6, SAFE_PASS_VARGS("Disk R/W: ", strdisk_read_usage, "MB/s ", "/ ", strdisk_write_usage, "MB/s "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    ftostr(proc_stat->Disk_read_mb_peak_usage, &strdisk_read_peak_usage);
    ftostr(proc_stat->Disk_write_mb_peak_usage, &strdisk_write_peak_usage);
    strconcat(
        &hdr,
        6,
        SAFE_PASS_VARGS("Disk peak R/W: ", strdisk_read_peak_usage, "MB/s ", "/ ", strdisk_write_peak_usage, "MB/s "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursY++;
    free(hdr);

    ulltostr(proc_stat->Disk_read_kb, &strdisk_read);
    ulltostr(proc_stat->Disk_written_kb, &strdisk_written);
    strconcat(&hdr, 6, SAFE_PASS_VARGS("Disk Read/Written: ", strdisk_read, "KB ", "/ ", strdisk_written, "KB "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    free(hdr);

    attroff(COLOR_PAIR(DEFAULT_PAIR));
    free(strdisk_read);
    free(strdisk_written);
    free(strdisk_read_usage);
    free(strdisk_write_usage);
    free(strdisk_read_peak_usage);
    free(strdisk_write_peak_usage);
  }
}

static void draw_menu(Window *win, int termX, int termY)
{
  UNUSED(termX);

  int cursX = 0,         // cursor X position
      cursY = termY - 1, // cursor Y position
      loffsetX = 4;      // left offset X position

  {
    attron(COLOR_PAIR(MENU_PAIR));

    char *hdr = NULL;
    strconcat(&hdr, 2, SAFE_PASS_VARGS(" F1 - Kill process "));
    mvwaddstr(win->__p, cursY, loffsetX, hdr);
    cursX += loffsetX + (int) strlen(hdr);
    free(hdr);

    strconcat(&hdr, 2, SAFE_PASS_VARGS(" F4 - Exit "));
    mvwaddstr(win->__p, cursY, loffsetX + cursX, hdr);
    cursX += loffsetX;
    free(hdr);

    attroff(COLOR_PAIR(MENU_PAIR));
  }
}

bool Window_refresh(Window *win, Process_stat *proc_stat)
{
  int x = COLS, y = LINES;
  {
#ifdef __linux__
    struct winsize sz; // get terminal real size at the moment
    if (ioctl(2, TIOCGWINSZ, &sz) == 0) {
      x = sz.ws_col;
      y = sz.ws_row;
    }
#elif _WIN32
    // TODO: size not updated after resize console
    CONSOLE_SCREEN_BUFFER_INFO out;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &out)) {
      x = out.srWindow.Right - out.srWindow.Left + 1;
      y = out.srWindow.Bottom - out.srWindow.Top + 1;
    }
#endif
  }
  wresize(win->__p, y, x);
  wrefresh(win->__p);
  clear();

  char *errormsg = NULL;
  if (!Process_stat_update(proc_stat, &errormsg)) {
    printw("%s\n", errormsg);
    printf("%s\n", errormsg);
    free(errormsg);
    return false;
  }
  draw_CPU_usage(win, proc_stat, x, y);
  draw_process_info(win, proc_stat, x, y);
  draw_menu(win, x, y);

  return true;
}

void Window_destroy(Window *win)
{
  endwin(); // remove ncurses WINDOW

  free(win);
}
