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
} GapBuffer;


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

/************************* #EDITOR ******************************/

typedef struct {
  uint16_t x, y;
  uint16_t rows, cols;
} Editor;


uint16_t ed_line_width(Editor *e, GapBuffer *g) {
  uint32_t old_point = g->point;
  uint32_t point_left = 0;
  
  if (g->point > 0 && gb_get_current(g) == 10) 
    g->point--;
  // MOVE LEFT
  for (; g->point > 0; g->point--) {
    if (gb_get_current(g) == 10) {
      point_left = g->point + 1;
    }
  }

  for (g->point = point_left; true; g->point++) {
    if (g->point >= g->size || gb_get_current(g) == 10) {
      uint32_t width = g->point - point_left;
      g->point = old_point;
      return width;
    }
  }
}

uint16_t ed_line_width_from_point(Editor *e, GapBuffer *g) {
  uint32_t old_point = g->point;
  for (; true; g->point++) {
    if (g->point >= g->size || gb_get_current(g) == 10) {
      uint32_t width = g->point - old_point;
      g->point = old_point;
      return width;
    }
  }
}

void editor_move_left(Editor *e, GapBuffer *g, WINDOW *area, uint8_t amount) {
  if (e->x > amount) {
    g->point -= amount;
    e->x -= amount;
  }
  else if (e->x > 0) {
    g->point -= e->x;
    e->x = 0;
  }
  else if (e->y > 0) {
    g->point -= 1;
    e->y -= 1;
    e->x = ed_line_width(e, g);
  }
  wmove(area, e->y, e->x);
}

void editor_move_right(Editor *e, GapBuffer *g, WINDOW *area, uint8_t amount) {
  
  uint16_t line_width = ed_line_width(e, g);
  
  if (g->point >= g->size - 1) {
    return;
  }
  else if (gb_get_current(g) == 10) {
    g->point++;
    e->y++;
    e->x = 0;
  }
  else if (e->x + amount > line_width) {
    g->point += e->x - line_width;
    e->x = line_width;
  }
  else {
    g->point += amount;
    e->x += amount;
  }
  wmove(area, e->y, e->x);
}

void editor_move_right_depr(Editor *e, GapBuffer *g, WINDOW *area) {
  if (gb_get_current(g) == 10) {
    e->y++;
    e->x = 0;
  }
  else {
    e->x++;
  }
  g->point++;
  wmove(area, e->y, e->x);
}

int read_file(Editor *e, GapBuffer *g, char* filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    die("File not found\n");
  }
    
  char buffer[1000]; // todo
  char c;
  for (int i = 0; (c = fgetc(file)) != EOF; i++) {
    buffer[i] = c;
  }
  //die("buffer: %d\n", strlen(buffer));
  g->size = strlen(buffer);
  memmove(g->buf + g->cap - g->size, buffer, g->size);
}

int print_status_line(WINDOW *statArea, Editor *e, GapBuffer *g, int c) {
  wmove(statArea, 0, 0);
  mvwprintw(statArea, 0, 0, "last: %d, ed: (%d, %d), width: %d, pos: %d, front: %d, C: %d, point: %d, size: %d \t\t", 
        c, e->y, e->x, ed_line_width(e, g), gb_pos(g), g->front, gb_get_current(g), g->point, g->size);
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
  Editor e = { .rows = 1, .x = 0, .y = 0};
  GapBuffer g = { .point = 0, .front = 0, .size = 0, .cap = 10000 };
  ASSERT(g.point < g.cap);
 
  g.buf = calloc(g.cap, sizeof(char));
  getmaxyx(stdscr, e.rows, e.cols);
  lineArea = newwin(e.rows - 1, 4, 0, 0);
  textArea = newwin(e.rows - 1, e.cols - 4, 0, 0);
  statArea = newwin(1, e.cols, 0, 0);
  mvwin(textArea, 0, 4);
  //vline(ACS_VLINE, e.rows); // ??
  mvwin(statArea, e.rows - 1, 0);
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
    read_file(&e, &g, argv[1]);
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
  print_status_line(statArea, &e, &g, 0);
  wrefresh(statArea);
  ASSERT(e.x == 0);
  ASSERT(e.y == 0);
  wmove(textArea, e.x, e.y);

  for (int i = 0; i < e.rows; i++) { 
    mvwprintw(lineArea, i, 0, "%d", i);
  }
  wrefresh(lineArea);
  
  ASSERT(g.point < g.cap);

  int c;
  while ((c = wgetch(textArea)) != STR_Q) {
    
    // if (c == KEY_UP || c == CTRL('i')) {
    if (c == KEY_UP) {
      if (e.y > 0) {
        //e.y -= 1;
        //wmove(textArea, e.y, e.x);
      }
    }
    
    else if (c == LK_DOWN) {
      if (e.y < e.rows) {
        //e.y += 1;
        //wmove(textArea, e.y, e.x);
      }
    } 

    else if (c == KEY_RIGHT) {
      editor_move_right(&e, &g, textArea, 1);
    }
    
    else if (c == KEY_LEFT) {
      editor_move_left(&e, &g, textArea, 1);
    }
    
    else if (c == LK_ENTER) {
      gb_jump(&g);
      g.buf[g.front] = '\n';
      g.size++;
      g.front++;
      g.point++;
      e.y += 1;
      e.x = 0;
      // e.rows += 1;
      // waddch(textArea, '\n');
    }
    
    else if (c >= 32 && c <= 126) {
      gb_jump(&g);
      // winsch(textArea, c);
      g.buf[g.front] = c;
      g.size++;
      g.front++;
      g.point++;
      e.x += 1;
    }

    // else if (c == 127) {
    //   g.buf[g.front] = 0;
    //   g.front--;
    //   e.x -= 1;
    //   wmove(textArea, e.y, e.x);
    // }
    print_status_line(statArea, &e, &g, c);
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
    wmove(textArea, e.y, e.x);
  }
  
  clear();
  endwin();
  return 0;
}
