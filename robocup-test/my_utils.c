/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include "my_utils.h"

const char *errormsg = "[Error]: ";
//const char *debugmsg = "[Debug]: ";
const char *debugmsg = "";
int mylog_init(int *logfd, const char *logfile)
{
    *logfd = open(logfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    return write(*logfd, "begin\n", 6);
}

/*
 * 将输出信息按printf的形式格式化输出到文件描述符fd
 * fd = 1 表示标准输出
 * fd = 2 表示标准错误
 * fd =-1 表示debug信息，可以用来调试,正式提交时将return的注释去掉，即可不输出
 * fd =-2 task
 * fd =-3 moto
 * fd =-4 sche
 * fd =-5 cmd
 * fd = -6 net
 * logfd 表示日志文件
 * 输出后会自动加上换行符
 */
int mylogfd(int fd, const char *format, ...)
{
//if (1)return 0;
    int ret = 0;
    if (fd == 2)
        ret = write (fd, errormsg, strlen(errormsg));

    //if (fd < 0) return;
    if (fd == -1) {
        fd = 1;
        ret = write (fd, debugmsg, strlen(debugmsg));
        return 0;
    } else if (fd == -2) {//task
        fd = 1;
        //return 0;
    } else if (fd == -3) {//moto
        fd = 1;
        return 0;
    } else if (fd == -4) {//sche
        fd = 1;
        //return 0;
    } else if (fd == -5) {//cmd
        fd = 1;
        //return 0;
    } else if (fd == -6) {//net
        fd = 1;
        //return 0;
    }
    static char buf[1024];
    va_list ap;

    va_start(ap, format);
    vsnprintf(buf, 1024, format, ap);
    va_end(ap);

    if (fd == 1)
        fd =2;
    ret = write(fd, buf, strlen(buf));
    ret = write(logfd, buf, strlen(buf));
    //ret = write(fd, "\n", 1);
    return ret;
}
char** strsplit_by_space (char *string, int *length)
{
    char *remainder, *s;
    int size = 8, num = 0, done = 0;
    char **array;

    if (string == NULL || string[0] == '\0') {
        *length = 0;
        return NULL;
    }

    array = malloc (sizeof(char *) * size);

    remainder = string;
    while (!done) {
        for (s = remainder; *s != ' ' && *s != '\0'; ++s) ;

        if (*s == '\0')
            done = 1;
        else
            *s = '\0';

        array[num++] = remainder;
        if (!done && num == size) {
            size <<= 1;
            array = realloc (array, sizeof(char *) * size);
        }

        remainder = s + 1;
    }

    *length = num;
    return array;
}
char** strsplit_by_char (char *string, int *length, char c)
{
    char *remainder, *s;
    int size = 8, num = 0, done = 0;
    char **array;

    if (string == NULL || string[0] == '\0') {
        *length = 0;
        return NULL;
    }

    array = malloc (sizeof(char *) * size);

    remainder = string;
    while (!done) {
        for (s = remainder; *s != c && *s != '\0'; ++s) ;

        if (*s == '\0')
            done = 1;
        else
            *s = '\0';

        array[num++] = remainder;
        if (!done && num == size) {
            size <<= 1;
            array = realloc (array, sizeof(char *) * size);
        }

        remainder = s + 1;
    }

    *length = num;
    return array;
}
int min(int a, int b)
{
    return a < b ? a : b;
}
int max(int a, int b)
{
    return a > b ? a : b;
}
struct cmd *getcmd (char *name, struct cmd* cmdtab) {
    char *p, *q;
    struct cmd *c, *found;
    int nmatches, longest;

    longest = 0;
    nmatches = 0;
    found = 0;
    for (c = cmdtab; (p = c->name); c++) {
        for (q = name; *q == *p++; q++)
            if (*q == 0)		/* exact match? */
                return c;
        if (!*q) {	/* the name was a prefix */
            if (q - name > longest) {
                longest = q - name;
                nmatches = 1;
                found = c;
            } else if (q - name == longest)
                nmatches++;
        }
    }

    if (nmatches > 1)
        return (struct cmd *)-1;
    return found;
}
