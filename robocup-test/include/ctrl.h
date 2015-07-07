#ifndef CTRL_H
#define CTRL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cmd.h"

static inline void print_buf(char *buf, int n)
{
    int i = 0;
    for (; i<n; ++i) printf("%hhx ", buf[i]);
    printf("\n");
}

static inline void copy_buf(char *dst, char *src, int n)
{
    int i = 0;
    for (; i<n; ++i) *(dst++) = *(src++);
}

static inline void ctrl_init(int fd)
{

    int n;
    char buf[4];

    printf("control program version %d\n", VERSION);

    printf("check version ...\n");
    buf[0] = CLR;
    buf[1] = VER;
    if (write(fd, buf, 2) != 2) {
        fprintf(stderr, "write error\n");
        exit(-1);
    }

    if ((n = read(fd, buf, sizeof(buf))) != 3) {
	printf("read n = %d\n", n);
        if (n == -1) {
            perror("read");
        } else {
            fprintf(stderr, "read error....\n");
            print_buf(buf, n);
        }
        exit(-1);
    }

    /*if (buf[1] != 3 || buf[2] != VERSION) {
    	fprintf(stderr, "version check error\n");

    	printf("%d",buf[2]);
    	print_buf(buf, 3);
    	exit(-1);
    }*/
    printf("version ok\n");

    printf("system init\n");
    buf[0] = STOP;
    if (write(fd, buf, 1) != 1) {
        fprintf(stderr, "write error\n");
        exit(-1);
    }
}

#endif

