/* #include <stdio.h> */
#include <fcntl.h>
#include <unistd.h>

#include "memset.h"
#include "shell.h"
#include "uart.h"

/* itoa impl reference: */
/* https://stackoverflow.com/questions/3982320/convert-integer-to-string-without-access-to-libraries */
char* itoa(int val, int base){
  static char buf[32] = {0};
  int i = 30;
  for(; val && i ; --i, val /= base)
    buf[i] = "0123456789abcdef"[val % base];
  return &buf[i+1];
}

void hello() {
  uart_puts("Hello World!\r\n");
}

int sstrcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

int getcmd(char *buf, int nbuf) {
  uart_puts("# ");

  memset(buf, 0, nbuf);
  char *p_buf = buf;

  /* read from uart to buf until newline */
  char c;
  while ((c = uart_getc()) != '\r') {
    if (c == 127 || c == 8) { /* backspace or delete */
      uart_puts("\b \b");
    } else {
      uart_send(c);
    }

    *buf++ = c;
  }
  uart_puts("\r\n");
  *buf = 0;

  return p_buf[0] == 0 ? -1 : 0;
}