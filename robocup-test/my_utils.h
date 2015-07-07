/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef _UTILS__H__
#define _UTILS__H__

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern int mylog_init (int *logfd, const char *logfile);
extern int mylogfd (int fd, const char *format, ...);
extern void mylog (const char *format, ...);
int logfd;
struct cmd {
    char *name;
    int (*handler) (int argc, char **argv);
};
extern struct cmd *getcmd (char *name, struct cmd* cmdtab);

extern char** strsplit_by_space (char *string, int *length);

extern char** strsplit_by_char (char *string, int *length, char c);

extern int min (int a, int b);

extern int max (int a, int b);

#endif
