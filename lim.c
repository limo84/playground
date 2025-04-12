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

typedef struct {
  char *buf;
	uint32_t size;
  uint32_t front;
  uint32_t gap;
  uint32_t point;
} GapBuffer;

typedef struct {
  uint16_t x, y;
  uint16_t rows, cols;
} Editor;

void die(const char *format, ...) {
  va_list args;
  va_start(args, format);
    vprintf(format, args);
  va_end(args);
 
  exit(0);
}

int read_file_to_buffer(GapBuffer *g, Editor *e, char* filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    exit(1);
  }
    
  char c;
  for (g->point = 0; (c = fgetc(file)) != EOF; g->point++) {
    g->buf[g->point] = c;
    //e->rows += (c == '\n') ? 1 : 0;
  }
}

int print_status_line(WINDOW *statArea, Editor e, GapBuffer g, int c) {
  wmove(statArea, 0, 0);
  attrset(COLOR_PAIR(2));
  mvwprintw(statArea, 0, 0, "c: %d, e: (%d, %d), f: %d, ls: %d, p: %d, C: %c, g: %d\t\t", 
        c, e.y, e.x, g.front, e.rows, g.point, g.buf[g.point], g.gap);
  attrset(COLOR_PAIR(1));
}

#define MIN(a, b) (a < b) ? (a) : (b)
#define MAX(a, b) (a > b) ? (a) : (b)

int gb_move_point(GapBuffer *g, int8_t direction) {
  bool in_front = (g->point <= g->front);
  g->point += direction;
	if (in_front && g->point > g->front) {
    g->point += g->gap;
  }
  else if (!in_front && g->point < g->front + g->gap) {
    g->point -= g->gap;
  }
  g->point = MAX(0, g->point);
  g->point = MIN(g->point, g->size);
}

int gb_jump(GapBuffer g) {
  if (g.point < g.front) {
    size_t n = g.front - g.point;
    memmove(g.buf + g.point + g.gap, g.buf + g.point, n);
  }
  else if (g.point > g.front + g.gap) {
		size_t n = g.point - (g.front + g.gap);
    memmove(g.buf + g.front, g.buf + g.point - n, n); 
  }
}

int main(int argc, char **argv) {

  initscr();
  start_color();
  atexit((void*)endwin);
  
  WINDOW *lineArea;
  WINDOW *textArea;
  WINDOW *statArea;
  Editor e = { .rows = 1 };
  GapBuffer g = { .point = 0, .front = 0, .gap = 10000, .size = 10000 };
	assert(g.point < g.size);
 
	g.buf = calloc(g.size, sizeof(char));
  getmaxyx(stdscr, e.rows, e.cols);
  lineArea = newwin(e.rows - 1, 4, 0, 0);
  textArea = newwin(e.rows - 1, e.cols - 4, 0, 0);
  statArea = newwin(1, e.cols, 0, 0);
  mvwin(textArea, 0, 4);
  vline(ACS_VLINE, e.rows); // ??
  mvwin(statArea, e.rows - 1, 0);
	assert(g.point < g.size);

  if (!has_colors()) {
    die("No Colors\n");
  }
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);
  wattrset(textArea, COLOR_PAIR(1));
  wattrset(statArea, COLOR_PAIR(2));
	assert(g.point < g.size);

  if (false && argc > 1) {
    read_file_to_buffer(&g, &e, argv[1]);
    wprintw(textArea, g.buf);
    wmove(textArea, 0, 0);
    wrefresh(textArea);
  }
  print_status_line(statArea, e, g, 0);
  wrefresh(statArea);
	assert(g.point < g.size);

  raw();
  keypad(textArea, TRUE);
  noecho();

  for (int i = 0; i < e.rows; i++) { 
    mvwprintw(lineArea, i, 0, "%d", i);
  }
  wrefresh(lineArea);
  
	assert(g.point < g.size);

  int c;
  while ((c = wgetch(textArea)) != STR_Q) {
    
    if (c == KEY_UP || c == CTRL('i')) {
      if (e.y > 0) {
        e.y -= 1;
        wmove(textArea, e.y, e.x);
      }
    }
    
    else if (c == LK_DOWN || c == CTRL('k')) {
      if (e.y < e.rows) {
        e.y += 1;
        wmove(textArea, e.y, e.x);
      }
    } 

    else if (c == KEY_RIGHT || c == CTRL('l')) {
      if (g.point < 10) {
        gb_move_point(&g, 1);
        e.x += 1;
        wmove(textArea, e.y, e.x);
      }
    }
    
    else if (c == KEY_LEFT || c == CTRL('j')) {
      if (e.x > 0) {
        gb_move_point(&g, -1);
        e.x -= 1;
        wmove(textArea, e.y, e.x);
      }
    }
    
    else if (c == CTRL('o')) {
      g.buf[g.front] = '\n';
      g.front++;
      e.y += 1;
      e.x = 0;
      e.rows += 1;
      waddch(textArea, c);
    }
    
    else if (c >= 32 && c <= 126) {
      gb_jump(g);
      winsch(textArea, c);
			g.buf[g.front] = c;
			g.gap--;
      g.front++;
      gb_move_point(&g, 1);
      e.x += 1;
    }

    else if (c == 127) {
      g.buf[g.front] = 0;
      g.front--;
      e.x -= 1;
      wmove(textArea, e.y, e.x);
    }
    
		//draw front
		wmove(textArea, 20, 0);
		wclrtoeol(textArea);
		mvwaddnstr(textArea, 20, 0, g.buf, g.front);
    // draw back
		wmove(textArea, 22, 0);
		wclrtoeol(textArea);
		uint32_t back = g.front + g.gap;
		mvwaddnstr(textArea, 22, 0, g.buf + back, g.size - back); 
    
		wrefresh(textArea);
    print_status_line(statArea, e, g, c);
    wrefresh(statArea);
    wmove(textArea, e.y, e.x); 
  }
  
  clear();
  endwin();
  return 0;
}
