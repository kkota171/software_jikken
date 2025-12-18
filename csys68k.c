extern void outbyte(unsigned char c, int port);
extern char inbyte(int port);

int readfd2port(int fd){
    switch (fd)
    {
    case 0:
        return 0; // UART1
    case 3:
        return 0; // UART1
    case 4:
        return 1; // UART2
    default:
        return -1; // 不明
    }
}

int writefd2port(int fd){
    switch (fd)
    {
    case 1:
        return 0; // UART1
    case 2:
        return 0; // UART1
    case 3:
        return 0; // UART1
    case 4:
        return 1; // UART2
    default:
        return -1; // 不明
    }
}

int read(int fd, char *buf, int nbytes)
{
  char c;
  int  i;
  int port = readfd2port(fd);

  for (i = 0; i < nbytes; i++) {
    c = inbyte(port);

    if (c == '\r' || c == '\n'){ /* CR -> CRLF */
      outbyte('\r', port);
      outbyte('\n', port);
      *(buf + i) = '\n';

    /* } else if (c == '\x8'){ */     /* backspace \x8 */
    } else if (c == '\x7f'){      /* backspace \x8 -> \x7f (by terminal config.) */
      if (i > 0){
	outbyte('\x8', port); /* bs  */
	outbyte(' ', port);   /* spc */
	outbyte('\x8', port); /* bs  */
	i--;
      }
      i--;
      continue;

    } else {
      outbyte(c, port);
      *(buf + i) = c;
    }

    if (*(buf + i) == '\n'){
      return (i + 1);
    }
  }
  return (i);
}

int write (int fd, char *buf, int nbytes)
{
  int i, j;
  int port = writefd2port(fd);
  for (i = 0; i < nbytes; i++) {
    if (*(buf + i) == '\n') {
      outbyte ('\r', port);          /* LF -> CRLF */
    }
    outbyte (*(buf + i), port);
    for (j = 0; j < 300; j++);
  }
  return (nbytes);
}
