#include <curses.h>

int main() {
  int c;
  initscr();
  keypad(stdscr, TRUE);
  noecho(); 
  endwin();
  return 0;
}
