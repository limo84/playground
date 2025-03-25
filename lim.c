#include <curses.h>
#include <stdint.h>
#include <stdlib.h>

#define STR_Q 17

#define LK_UP 65
#define LK_DOWN 66
#define LK_RIGHT 67
#define LK_LEFT 68

typedef struct {
  char *buf;
  uint32_t front;
  uint32_t back;
  uint32_t p;
} GapBuffer;

typedef struct {
  uint16_t x, y;
  uint16_t row;
} Editor;

int main() {
  
  Editor e = { 0 };
  GapBuffer g = { 0 };
  g.buf = malloc(10000);
  int c;

  initscr();
  raw();
  //cbreak();
  keypad(stdscr, TRUE);
  noecho();

  while ((c = getch()) != STR_Q) {
    
    if (c == 10) {
      g.buf[g.front] = '\n';
      g.front++;
      e.y += 1;
      e.x = 0;
      e.row += 1;
    }

    if (c == KEY_UP) {
      if (e.y > 0) {
          e.y -= 1;
      }
    }
    
    if (c == LK_DOWN) {
      if (e.y < e.row) {
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
    }

    if (c == 127) {
      g.buf[g.front] = 0;
      g.front--;
      e.x -= 1;
    }
    
    clear();
    refresh();
    printw(g.buf);
    move(10, 0);
    printw("c: %d, e: (%d, %d), f: %d", c, e.y, e.x, g.front);
    move(e.y, e.x);
  }
  
  clear();
  move(0, 0);
  endwin();
  return 0;
}
