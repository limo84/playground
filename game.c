#include <curses.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/time.h>


int main() {
  WINDOW *textArea;
  int c, rows, cols;
  struct timeval tp;
  unsigned long millis = 1000, old_millis = 1000;
	unsigned long delta = 1000;

  char buffer[100] = "asdasdasdasd";
  unsigned int index = 0;

  // system("setfont lat0-08.psfu.gz");

  initscr();
  atexit((void*)endwin);

  raw();
  keypad(textArea, TRUE);
  noecho();

  getmaxyx(stdscr, rows, cols);
  textArea = newwin(rows, cols, 0, 0);

  millis = tp.tv_sec * 1000 + tp.tv_usec / 1000;

  // mvwprintw(textArea, 0, 0, "%s", buffer);
  wrefresh(textArea);

  // while ((c = getch()) != 'q') {
  //   gettimeofday(&tp, NULL);
  //   millis = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  //   delta = millis - old_millis;

  //   // switch (c) {
  //   //   case KEY_DOWN:
  //   //     printw ("KEY_DOWN : %d <-> %lu\n", c, delta);
  //   //     break;
  //   //   default:
  //   //     printw ("Tastaturcode %d\n",c);
  //   // }

  //   /** TEST */ 

  //   switch (c) {
  //     default:
  //       buffer[index++] = c; 
  //   }

  //   // wclear(textArea);
  //   mvwprintw(textArea, 0, 0, "%s", buffer);
  //   wrefresh(textArea);

  //   old_millis = millis;

  // }
  endwin();
  // system("setfont default8x16.psfu.gz");

  return 0;
}

