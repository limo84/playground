#include <curses.h>
#include <stdint.h>
#include <stdlib.h>

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

int count_rows() {
  for (int i = 1; i < 100; i++) {
    printw("%d", i);
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
  printf("window has 200 cols ???\n");
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

//void die(char *str)

int main(int argc, char **argv) {

  initscr();
  start_color();
  
  Editor e = { .rows = 1 };

  e.cols = count_cols();  
  e.rows = count_rows();
  
  // die!!!
  //printf("rows: %d. cols: %d\n", e.rows, e.cols); endwin(); exit(0);

  if (!has_colors()) {
    printf("No Colors\n");
    exit(1);
  }
  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);
  attrset(COLOR_PAIR(1));

  GapBuffer g = { .gap = 10000 };
  g.buf = malloc(10000);

  if (argc > 1) {
    read_file_to_buffer(&g, &e, argv[1]);
    printw(g.buf);
    move(0, 0);
    refresh();
  }
  print_status_line(&e, g, 0);

  raw();
  keypad(stdscr, TRUE);
  noecho();
  
  int c;
  while ((c = getch()) != STR_Q) {
    
    if (c == 10) {
      g.buf[g.front] = '\n';
      g.front++;
      e.y += 1;
      e.x = 0;
      e.rows += 1;
    }

    if (c == KEY_UP) {
      if (e.y > 0) {
          e.y -= 1;
      }
    }
    
    if (c == LK_DOWN) {
      if (e.y < e.rows) {
    //exit(0);
        e.y += 1;
      }
    } 

    if (c == KEY_RIGHT) {
      if (e.x < 10) {
        e.x += 1;
      }
    }
    
    if (c == KEY_LEFT) {
      if (e.x > 0) {
        e.x -= 1;
      }
    }

    if (c >= 'a' && c <= 'z') {
      g.buf[g.front] = c;
      g.front++;
      e.x += 1;
      addch(c);
      //clear();
      //printw(g.buf);
    }

    if (c == 127) {
      g.buf[g.front] = 0;
      g.front--;
      e.x -= 1;
    }
    
    //clear();
    refresh();
    //printw(g.buf);
    print_status_line(&e, g, c);
  }
  
  clear();
  //move(0, 0);
  endwin();
  return 0;
}
