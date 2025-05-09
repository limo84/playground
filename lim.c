#include <curses.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

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
  return g->buf[gb_pos(g)];
}

int gb_jump(GapBuffer *g) {
  if (g->front > g->point) {
    size_t n = g->front - g->point;
    memmove(g->buf + g->point + gb_gap(g), g->buf + g->point, n);
    g->front = g->point;
  }
  else if (g->point > g->front) {
    size_t n = g->point - g->front;
    memmove(g->buf + g->front, g->buf + g->point - n, n);
    g->front = g->point;
  }
}

void gb_refresh_line_width(GapBuffer *g) {
  uint32_t old_point = g->point;
  uint32_t point_right = 0;
  uint32_t point_left = 0;
  
  //if (g->point > 0 && gb_get_current(g) == 10) // 
  //  g->point--;
  
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
  //die("buffer: %d\n", strlen(buffer));
  g->size = strlen(buffer);
  memmove(g->buf + g->cap - g->size, buffer, g->size);
  gb_refresh_line_width(g);
  return 0;
}

int print_status_line(WINDOW *statArea, GapBuffer *g, int c) {
  wmove(statArea, 0, 0);
  mvwprintw(statArea, 0, 0, "last: %d, ed: (%d, %d), width: %d, pos: %d, front: %d, C: %d, point: %d, "
      "size: %d, lstart: %d, lend: %d, maxl: %d \t\t\t",
      c, g->lin + 1, g->col + 1, g->line_width, gb_pos(g), g->front, gb_get_current(g), g->point, g->size, 
      g->line_start, g->line_end, g->maxlines);
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
  Screen screen;
  GapBuffer g;
  gb_init(&g, 10000);                     
  ASSERT(g.point < g.cap);
 
  g.buf = calloc(g.cap, sizeof(char));
  getmaxyx(stdscr, screen.rows, screen.cols);
  lineArea = newwin(screen.rows - 1, 4, 0, 0);
  textArea = newwin(screen.rows - 1, screen.cols - 4, 0, 0);
  statArea = newwin(1, screen.cols, 0, 0);
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
  wattrset(textArea, COLOR_PAIR(1));
  wattrset(statArea, COLOR_PAIR(2));
  ASSERT(g.point < g.cap);

  raw();
  keypad(textArea, TRUE);
  noecho();
  
  if (argc > 1) {
    read_file(&g, argv[1]);
    wmove(textArea, 0, 0);
    wclear(textArea);
    mvwaddnstr(textArea, 0, 0, g.buf, g.front);
    // draw back
    uint32_t back = g.cap - g.size + g.front;
    wrefresh(textArea);
    wattrset(textArea, COLOR_PAIR(3));
    waddnstr(textArea, g.buf + back, g.cap - back); 
    wattrset(textArea, COLOR_PAIR(1));

    wrefresh(textArea);
  }
  print_status_line(statArea, &g, 0);
  wrefresh(statArea);
  wmove(textArea, 0, 0);

  // TODO make function
  for (int i = 1; i < g.maxlines; i++) { 
    mvwprintw(lineArea, i - 1, 0, "%d", i);
  }
  wrefresh(lineArea);
  
  ASSERT(g.point < g.cap);

  int c;
  while ((c = wgetch(textArea)) != STR_Q) {
    
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
    
    else if (c == LK_ENTER) {
      gb_jump(&g);
      g.buf[g.front] = '\n';
      g.size++;
      g.front++;
      g.point++;
      g.lin += 1;
      g.col = 0;
      gb_refresh_line_width(&g);
    }
    
    else if (c >= 32 && c <= 126) {
      gb_jump(&g);
      // winsch(textArea, c);
      g.buf[g.front] = c;
      g.size++;
      g.front++;
      g.point++;
      g.lin += 1;
    }

    // else if (c == 127) {
    // }

    print_status_line(statArea, &g, c);
    wrefresh(statArea);
    //draw front
    wmove(textArea, 0, 0);
    wclear(textArea);
    mvwaddnstr(textArea, 0, 0, g.buf, g.front);
    // draw back
    uint32_t back = g.cap - g.size + g.front;
    wrefresh(textArea);
    wattrset(textArea, COLOR_PAIR(3));
    waddnstr(textArea, g.buf + back, g.cap - back); 
    wattrset(textArea, COLOR_PAIR(1));
    wrefresh(textArea);
    wmove(textArea, g.lin, g.col);
  }
  
  clear();
  endwin();
  return 0;
}
