#include <curses.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

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
  uint32_t p;
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

int count_rows() {
  for (int i = 1; i < 100; i++) {
    if (move(i, 0) == ERR) {
      move(0, 0);
      return i;
    }
  }
  printf("window has 100 rows ???\n");
  endwin();
  exit(0);
}

int count_cols() {
  for (int i = 1; i < 500; i++) {
    if (move(0, i) == ERR) {
      move(0, 0);
      return i;
    }
  }
  printf("window has 500 cols ???\n");
  endwin();
  exit(0);
}

int read_file_to_buffer(GapBuffer *g, Editor *e, char* filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    exit(1);
  }
    
  char c;
  for (g->p = 0; (c = fgetc(file)) != EOF; g->p++) {
    g->buf[g->p] = c;
    //e->rows += (c == '\n') ? 1 : 0;
  }
}

int print_status_line(Editor *e, GapBuffer g, int c) {
  //move(e.rows - 1, 0);
  //printw("                                                                                                       ");
  move(e->rows - 1, 0);
  attrset(COLOR_PAIR(2));
  printw("c: %d, e: (%d, %d), f: %d, lines: %d", c, e->y, e->x, g.front, e->rows);
  attrset(COLOR_PAIR(1));
  //move(e.y, e.x);
}


int main(int argc, char **argv) {

  initscr();
  start_color();
  atexit((void*)endwin);
  
  WINDOW *lineArea;
  WINDOW *textArea;
  WINDOW *statArea;

  Editor e = { .rows = 1 };

  //e.cols = count_cols();  
  //e.rows = count_rows();

  getmaxyx(stdscr, e.rows, e.cols);

  lineArea = newwin(e.rows - 1, 3, 0, 0);
  textArea = newwin(e.rows - 4, e.cols - 3, 0, 0);
  statArea = newwin(1, e.cols, 0, 0);
  mvwin(textArea, 0, 3);
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

  GapBuffer g = { .gap = 10000 };
  g.buf = malloc(10000);

  if (false && argc > 1) {
    read_file_to_buffer(&g, &e, argv[1]);
    wprintw(textArea, g.buf);
    wmove(textArea, 0, 0);
    wrefresh(textArea);
  }
  //print_status_line(&e, g, 0);

  raw();
  keypad(textArea, TRUE);
  noecho();

  for (int i = 1; i < e.rows; i++) { 
    wprintw(lineArea, "%d\n", i);
  }
  wrefresh(lineArea);
  
  wprintw(statArea, "asdasdasdasd");
  wrefresh(statArea);
  
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
      if (e.x < 10) {
        e.x += 1;
        wmove(textArea, e.y, e.x);
      }
    }
    
    if (c == KEY_LEFT) {
      if (e.x > 0) {
        e.x -= 1;
        wmove(textArea, e.y, e.x);
      }
    }

    if (c >= 32 && c <= 126) {
      g.buf[g.front] = c;
      g.front++;
      e.x += 1;
      waddch(textArea, c);
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
    //print_status_line(&e, g, c);
  }
  
  clear();
  //move(0, 0);
  endwin();
  return 0;
}
