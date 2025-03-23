#include <curses.h>

int main() {
  int c;
  initscr();
  keypad(stdscr, TRUE);
  noecho();
  while ((c = getch()) != 'q') {
    switch (c) {
      case KEY_DOWN:
        printw ("KEY_DOWN : %d\n", c);
        break;
      default:
        printw ("Tastaturcode %d\n",c);
    }
  }
  endwin();
  return 0;
}
