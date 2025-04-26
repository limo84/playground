#include <curses.h>
#include <unistd.h>
#include <stdlib.h>


int main() {
  int c;
  system("setfont lat0-08.psfu.gz");
  //execl("/usr/bin/setfont", "setfont", "default8x16.psfu.gz", NULL);
  //execl("/usr/bin/setfont", "setfont", "lat0-08.psfu.gz", NULL);
  //exit(0);
  // execv("setfont lat0-08.psfu.gz", NULL);
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
  //execl("/usr/bin/setfont", "setfont", "default8x16.psfu.gz", NULL);
  system("setfont default8x16.psfu.gz");

  return 0;
}
//setfont default8x16.psfu.gz
