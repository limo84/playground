#include <curses.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <stdio.h>

#define STR_Q 17
#define LK_ENTER 10
#define LK_UP 65
#define LK_DOWN 258
#define LK_RIGHT 67
#define LK_LEFT 68

typedef struct {
  char *buf;
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
  mvwprintw(statArea, 0, 0, "c: %d, e: (%d, %d), f: %d, lines: %d, point: %d, current: %c\t\t", 
        c, e.y, e.x, g.front, e.rows, g.point, g.buf[g.point]);
  attrset(COLOR_PAIR(1));
}

#define MIN(a, b) (a < b) ? (a) : (b)
#define MAX(a, b) (a > b) ? (a) : (b)

int gb_move_point(GapBuffer *g, int8_t direction) {
  
  bool in_front = (g->point <= g->front);
  g->point += direction;
  //g.pointer = MIN(g.pointer, MAXLENGTH);
  if (in_front && g->point > g->front) {
    g->point += g->gap;
  }
  else if (!in_front && g->point < g->front + g->gap) {
    g->point -= g->gap;
  }

  g->point = MAX(0, g->point);
}

int gb_jump(GapBuffer g) {
  if (g.point < g.front) {
    size_t n = g.point - g.front;
    memmove(g.buf + g.point, g.buf + g.point + g.gap, n);
  }
  else if (g.point > g.front + g.gap) {
    //memmove(g.buf + g.front + g.gap, g.bu //TODO 
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
  GapBuffer g = { .gap = 10000 };

  g.buf = calloc(10000, sizeof(char));
  //e.cols = count_cols();  
  //e.rows = count_rows();

  getmaxyx(stdscr, e.rows, e.cols);

  lineArea = newwin(e.rows - 1, 4, 0, 0);
  textArea = newwin(e.rows - 1, e.cols - 4, 0, 0);
  statArea = newwin(1, e.cols, 0, 0);
  mvwin(textArea, 0, 4);
  vline(ACS_VLINE, e.rows); // ??
  mvwin(statArea, e.rows - 1, 0);
  
  
  /** die!!! */
  //die("rows: %d. cols: %d\n", e.rows, e.cols);

  if (!has_colors()) {
    die("No Colors\n");
  }
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);
  wattrset(textArea, COLOR_PAIR(1));
  wattrset(statArea, COLOR_PAIR(2));


  if (false && argc > 1) {
    read_file_to_buffer(&g, &e, argv[1]);
    wprintw(textArea, g.buf);
    wmove(textArea, 0, 0);
    wrefresh(textArea);
  }
  print_status_line(statArea, e, g, 0);

  raw();
  keypad(textArea, TRUE);
  noecho();

  for (int i = 0; i < e.rows; i++) { 
    mvwprintw(lineArea, i, 0, "%d", i + 101);
  }
  wrefresh(lineArea);
  
  //wprintw(statArea, "asdasdasdasd");
  //wrefresh(statArea);
  
  int c;
  while ((c = wgetch(textArea)) != STR_Q) {
    
    if (c == LK_ENTER) {
      g.buf[g.front] = '\n';
      g.front++;
      e.y += 1;
      e.x = 0;
      e.rows += 1;
      waddch(textArea, c);
    }

    if (c == KEY_UP) {
      if (e.y > 0) {
        e.y -= 1;
        wmove(textArea, e.y, e.x);
      }
    }
    
    if (c == LK_DOWN) {
      if (e.y < e.rows) {
        e.y += 1;
        wmove(textArea, e.y, e.x);
      }
    } 

    if (c == KEY_RIGHT) {
      if (g.point < 10) {
        gb_move_point(&g, 1);
        e.x += 1;
        wmove(textArea, e.y, e.x);
      }
    }
    
    if (c == KEY_LEFT) {
      if (e.x > 0) {
        gb_move_point(&g, -1);
        e.x -= 1;
        wmove(textArea, e.y, e.x);
      }
    }

    if (c >= 32 && c <= 126) {
      g.buf[g.front] = c;
      g.front++;
      gb_move_point(&g, 1);
      e.x += 1;
			winsch(textArea, c);
      //waddch(textArea, c);
      //wclear(textArea);
      //printw(g.buf);
    }

    if (c == 127) {
      g.buf[g.front] = 0;
      g.front--;
      e.x -= 1;
      wmove(textArea, e.y, e.x);
    }
    
    //clear();
    wrefresh(textArea);
    //printw(g.buf);
    print_status_line(statArea, e, g, c);
    wrefresh(statArea);
    wmove(textArea, e.y, e.x); 
  }
  
  clear();
  //move(0, 0);
  endwin();
  return 0;
}
