#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static inline void msave(char *filename, unsigned char *src, int size)
{

    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror(filename);
        exit(-1);
    }

    if (lseek(fd, size - 1, SEEK_SET) < 0) {
        perror("lseek");
        exit(-1);
    }

    if (write(fd, "", 1) != 1) {
        fprintf(stderr, "write error\n");
        exit(-1);
    }

    unsigned char *dst = (unsigned char *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (dst == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }

    memcpy(dst, src, size);

    if (munmap(dst, size) < 0) {
        perror("munmap");
        exit(-1);
    }
    close(fd);
}

#endif

