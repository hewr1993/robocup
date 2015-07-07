#ifndef TTY_H
#define TTY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

static inline int ttys_init(int id)
{

    if (id < 0 || id > 9) {
        fprintf(stderr, "ttys id should be 0 to 9\n");
        exit(-1);
    }

    char dev[15];
    sprintf(dev, "/dev/ttyUSB%d", id);
	
    int fd;
    if ((fd = open(dev, O_RDWR | O_NOCTTY)) < 0) {
        perror(dev);
        return -1;
    }
    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = B19200 | CS8 | CLOCAL | CREAD | PARENB;
    tio.c_iflag = INPCK;
    tio.c_cc[VMIN] = 1;

    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        perror("tcsetattr");
        return -1;
    }
    return fd;
}

static inline void tty_set(struct termios *tio)
{

    struct termios t;

    tcgetattr(0, &t);
    *tio = t;

    t.c_lflag = ISIG;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;

    tcflush(0, TCIFLUSH);
    tcsetattr(0, TCSANOW, &t);
}

#endif

