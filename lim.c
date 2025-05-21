// TODO Backspace at zero
// TODO Bug in gb_backspace
// TODO refresh line numbers
// TODO coredumps
// TODO load file from opened editor

// TODO list files with dirent, then choose

#include <curses.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include <dirent.h>

#include <stdio.h>

#define ASSERT(c) assert(c)

#define CTRL(c) ((c) & 037)
#define STR_Q 17
#define LK_ENTER 10
#define LK_UP 65
#define LK_DOWN 258
#define LK_RIGHT 67
#define LK_LEFT 68

#define MIN(a, b) (a < b) ? (a) : (b)
#define MAX(a, b) (a > b) ? (a) : (b)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// ********************* #MISC *********************************/

void die(const char *format, ...) {
  va_list args;
  va_start(args, format);
    vprintf(format, args);
  va_end(args);
 
  exit(0);
}

/******************** #GAPBUFFER *******************************/

typedef struct {
  char *buf;
  uint32_t cap;
  uint32_t size;
  uint32_t front;
  uint32_t point;
  uint32_t line_start;
  uint32_t line_end;
  uint16_t line_width;
  uint16_t lin, col;
  uint16_t maxlines;
} GapBuffer;


void gb_init(GapBuffer *g, uint32_t init_cap) {
  g->cap = init_cap;
  g->size = 0;
  g->front = 0;
  g->point = 0;
  g->line_width = 0;
  g->maxlines = 1;
}

uint32_t gb_gap(GapBuffer *g) {
  return g->cap - g->size;
}

uint32_t gb_pos(GapBuffer *g) {
  return g->point + (g->point >= g->front) * gb_gap(g);
}

uint32_t gb_pos_offset(GapBuffer *g, uint32_t offset) {
  uint32_t n = g->point + offset;
  return n + (n >= g->front) * gb_gap(g);
}

char gb_get_current(GapBuffer *g) {
  u32 pos = gb_pos(g);
  ASSERT(pos >= 0);
  ASSERT(pos < g->cap);
  return g->buf[pos];
}

int gb_jump(GapBuffer *g) {
  // MOVE DATA TO END
  if (g->point < g->front) {
    size_t n = g->front - g->point;
    memmove(g->buf + g->point + gb_gap(g), g->buf + g->point, n);
    g->front = g->point;
  }
  // MOVE DATA TO FRONT 
  else if (g->point > g->front) {
    size_t n = g->point - g->front;
    memmove(g->buf + g->front, g->buf + gb_gap(g) + g->front, n);
    g->front = g->point;
  }
}

void gb_refresh_line_width(GapBuffer *g) {
  uint32_t old_point = g->point;
  uint32_t point_right = 0;
  uint32_t point_left = 0;
  
  // MOVE RIGHT
  for (; g->point < g->size; g->point++) {
    if (gb_get_current(g) == 10) {
      point_right = g->point;
      break;
    }
  }

  // MOVE LEFT
  for (g->point = point_right - 1; g->point > 0; g->point--) {
    if (gb_get_current(g) == 10) {
      point_left = g->point;
      break;
    }
  }

  g->line_start = point_left + (point_left == 0 ? 0 : 1);
  g->line_end = point_right;    
  g->line_width = g->line_end - g->line_start + 1;
  g->point = old_point;
}

u16 gb_prev_line_width(GapBuffer *g) {
  
  if (g->lin == 0)
    return 0;
  if (g->col != 0)
    return 0;

  g->point--;
  gb_refresh_line_width(g);
  u16 prev_line_width = g->line_end - g->line_start;
  g->point++;
  gb_refresh_line_width(g);
  
  return prev_line_width;
}

bool gb_backspace(GapBuffer *g) {
  
  if (g->col == 0 && g->lin == 0) {
    return false;
  }

  if (g->col > 0) {
    g->col--;
  } 
  else {
    g->lin--;
    g->maxlines--;
    g->col = gb_prev_line_width(g);
  }
  gb_jump(g);
  g->point--;
  g->size--;
  g->front--;
  gb_refresh_line_width(g);
  return true;
}

void gb_move_right(GapBuffer *g) {
  if (g->point >= g->size - 1) {
    return;
  }
  if (gb_get_current(g) == 10) {
    g->point++;
    g->lin++;
    g->col = 0;
    gb_refresh_line_width(g);
  }
  else {
    g->point++;
    g->col++;
  }
}

void gb_move_left(GapBuffer *g) {
  if (g->point <= 0) {
    return;
  }
  g->point--;
  if (gb_get_current(g) == 10) {
    gb_refresh_line_width(g);
    g->lin--;
    g->col = g->line_width - 1;
  }
  else {
    g->col--;
  }
}

void gb_move_up(GapBuffer *g) {
  if (g->lin == 0) {
    return;
  }
  g->point = g->line_start - 1;
  gb_refresh_line_width(g);
  g->lin--;
  g->col = MIN(g->col, g->line_width - 1);
  g->point -= (g->col < g->line_width - 1) ? 
      g->line_width - g->col - 1 : 0; 
}

void gb_move_down(GapBuffer *g) {
  if (g->lin >= g->maxlines - 2) { // TODO ?
    return;
  }
  g->point = g->line_end + 1;
  gb_refresh_line_width(g);
  g->lin++;
  g->col = MIN(g->col, g->line_width - 1);
  g->point += g->col;
}

/************************* #EDITOR ******************************/

typedef struct {
  uint16_t rows, cols;
} Screen;

int read_file(GapBuffer *g, char* filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    die("File not found\n");
  }
    
  char buffer[1000]; // TODO seek actual number of chracters before reading to gapbuffer
  g->maxlines = 1;
  char c;
  for (int i = 0; (c = fgetc(file)) != EOF; i++) {
    buffer[i] = c;
    if (c == 10) {
      g->maxlines++;
    }
  }
  g->size = strlen(buffer);
  memmove(g->buf + g->cap - g->size, buffer, g->size);
  gb_refresh_line_width(g);
  return 0;
}

void print_text_area(WINDOW *textArea, GapBuffer *g) {
  wmove(textArea, 0, 0);
  wclear(textArea);
  u32 point = g->point;
  for (g->point = 0; g->point < g->size; g->point++) {
    waddch(textArea, gb_get_current(g));
  }
  g->point = point;
}

void draw_line_area(GapBuffer *g, WINDOW *lineArea) {
  wclear(lineArea);
  for (int i = 1; i < g->maxlines; i++) { 
    mvwprintw(lineArea, i - 1, 0, "%d", i);
  }
  wrefresh(lineArea);
}

int print_status_line(WINDOW *statArea, GapBuffer *g, int c) {
  wmove(statArea, 0, 0);
  mvwprintw(statArea, 0, 0, "last: %d, ", c);
  wprintw(statArea, "ed: (%d, %d), ", g->lin + 1, g->col + 1);
  //wprintw(statArea, "width: %d, ", g->line_width);
  wprintw(statArea, "pos: %d, ", gb_pos(g));
  //wprintw(statArea, "front: %d, ", g->front);
  //wprintw(statArea, "C: %d, ", gb_get_current(g));
  //wprintw(statArea, "point: %d, ", g->point);
  //wprintw(statArea, "size: %d, ", g->size);
  //wprintw(statArea, "lstart: %d, ", g->line_start);
  //wprintw(statArea, "lend: %d, ", g->line_end);
  wprintw(statArea, "maxl: %d, ", g->maxlines);
  wprintw(statArea, "prev: %d, ", gb_prev_line_width(g));
  //wprintw(statArea, "\t\t\t");
//   ,  "
 //     ",  ,
   //   c, , , , , , , , 
   //   , , );
}

int main(int argc, char **argv) {

  initscr();
  start_color();
  atexit((void*)endwin);

  struct timeval tp;
  unsigned long millis = 1000, old_millis = 1000;
  unsigned long delta = 1000;
  
  WINDOW *lineArea;
  WINDOW *textArea;
  WINDOW *statArea;
  WINDOW *popupArea;
  Screen screen;
  GapBuffer g;
  gb_init(&g, 10000);                     
  ASSERT(g.point < g.cap);
 
  g.buf = calloc(g.cap, sizeof(char));
  getmaxyx(stdscr, screen.rows, screen.cols);
  lineArea = newwin(screen.rows - 1, 4, 0, 0);
  textArea = newwin(screen.rows - 1, screen.cols - 4, 0, 0);
  statArea = newwin(1, screen.cols, 0, 0);
  
  popupArea = newwin(5, 30, 10, 10);
  
  mvwin(textArea, 0, 4);
  //vline(ACS_VLINE, screen.rows); // ??
  mvwin(statArea, screen.rows - 1, 0);
  ASSERT(g.point < g.cap);

  if (!has_colors()) {
    die("No Colors\n");
  }
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);
  init_pair(3, COLOR_RED, COLOR_BLACK);
  init_pair(4, COLOR_BLACK, COLOR_RED);
  wattrset(textArea, COLOR_PAIR(1));
  wattrset(statArea, COLOR_PAIR(4));
  ASSERT(g.point < g.cap);

  raw();
  keypad(textArea, TRUE);
  noecho();
  
  if (argc > 1) {
    read_file(&g, argv[1]);
    print_text_area(textArea, &g);
    wrefresh(textArea);
  }
  //print_status_line(statArea, &g, 0);
  wrefresh(statArea);
  wmove(textArea, 0, 0);

  draw_line_area(&g, lineArea);
  
  ASSERT(g.point < g.cap);

  int c;
  bool changed;
  while ((c = wgetch(textArea)) != STR_Q) {
    
    changed = false;
    // if (c == KEY_UP || c == CTRL('i')) {
    if (c == KEY_UP) {
      gb_move_up(&g);
    }
    
    else if (c == LK_DOWN) {
      gb_move_down(&g);
    } 

    else if (c == KEY_RIGHT) {
      gb_move_right(&g);
    }
    
    else if (c == KEY_LEFT) {
      gb_move_left(&g);
    }

    else if (c == 263) {
      changed = gb_backspace(&g);
      draw_line_area(&g, lineArea);
    }
    
    else if (c == LK_ENTER) {
      gb_jump(&g);
      g.buf[g.front] = '\n';
      g.size++;
      g.front++;
      g.point++;
      g.lin += 1;
      g.col = 0;
      g.maxlines++;
      draw_line_area(&g, lineArea);
  
      gb_refresh_line_width(&g);
      changed = true;
    }
    
    else if (c >= 32 && c <= 126) {
      gb_jump(&g);
      // winsch(textArea, c);
      g.buf[g.front] = c;
      g.size++;
      g.front++;
      g.point++;
      g.col += 1;
      gb_refresh_line_width(&g);
      changed = true;
    }

    // else if (c == 127) {
    // }

    print_status_line(statArea, &g, c);
    wrefresh(statArea);
    // draw front
    if (changed) {
      print_text_area(textArea, &g);
    }
    wrefresh(textArea);
    wmove(textArea, g.lin, g.col);
    
    //wresize(popupArea, 30, 60);
    //wbkgd(popupArea, COLOR_PAIR(2));
    //box(popupArea, ACS_VLINE, ACS_HLINE);
    //wrefresh(popupArea);
  }
  
  clear();
  endwin();
  return 0;
}
