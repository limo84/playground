#include <stdio.h>

int main() {
  for (int i = 27; i < 256; i++) {
    printf ("%d: %c\t", i, (char)i);
    if ((i - 1) % 10 == 0) {
      printf ("\n");
    }
  }
}
