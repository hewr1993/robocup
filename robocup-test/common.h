/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>

#include "cmd.h"
#include "tty.h"
#include "ctrl.h"
#include "net.h"

#include "my_utils.h"

#define MOTOR_ON 1
#define DEFAULT_ANGLESEG 2
#define SEGDIS 2
#define DEFAULT_A 30
#define MAX_A 40
#define MAXV 70

#define PORT 30001
#define BUFSIZE 512
#define LENGTH 250
#define WIDTH 350
#define DOOR 120
#define DD 30.1



#define COLOR_DEPTH 256
#define INT_BITS 32
#define PI 3.1415926
#define MAX_LINES 10

#define LOC_LEFT 2000
#define LOC_RIGHT -2000
#define LOC_INVALID -10000
#define LOC_CATCH_BALL 10000
#define LOC_Y_FAR    300
#define LOC_Y_INVALID 9999
#define LOC_Y_NEAR   1000
#define LOC_Y_VERY_NEAR 500

#define ARC_TO_DEGREE(x) ((x)*180/PI)
#define DEGREE_TO_ARC(x) ((x)*PI/180)


typedef struct _Point {
    int x;
    int y;
} Point;
extern const Point UNKNOW ;
extern int logfd;
enum DEBUG_FLAG {
    FLAG_NONE = 0,
    FLAG_VIDEO = 1,
    FLAG_NET = 1 << 1,
    FLAG_SCHED = 1 << 2,
    FLAG_ALL = 7
};


typedef struct Conf {
    int turn_speed_s;
    int turn_speed;
    int face_to_ball_speed;
    int go_forward_speed_q;
    int go_forward_speed_m;
    int go_forward_speed_s;
} Conf;

static inline void err_sys(const char* msg)
{
    perror(msg);
    exit(-1);
};

typedef struct Trace {
    int pnum;//点数
    Point *ps;//轨迹点
    int *direction;//点方向
    int *inverse;//是否反向，仅在两点直线运动时考虑
    int *r;
    //double *rate;//左右轮速度比
} Trace;

static inline void freeTrace (Trace *tr)
{
    free (tr->ps);
    free (tr->r);
    free (tr->inverse);
    free (tr->direction);
    free (tr);
}
static inline Trace* newTrace (int num)
{
    //int i = 0;
    Trace *tr = malloc (sizeof(Trace));
    tr->pnum = num;
    //tr->rate = malloc (sizeof(double) * num);
    //for (i=0; i< num; ++i)         tr->rate[i] = 1;
    tr->ps=  malloc (sizeof(Point) * num);
    memset (tr->ps, 0, sizeof(Point) * num);
    tr->inverse = malloc (sizeof(int) * num);
    memset (tr->inverse, 0, sizeof(int) * num);
    tr->r = malloc (sizeof(int) * num);
    memset (tr->r, 0, sizeof(int) * num);
    tr->direction = malloc (sizeof(int) * num);
    memset (tr->direction, 0, sizeof(int) * num);
    return tr;
}

static inline int getAngle (Point p)
{
    if (p.y == 0)
        return 0;
    int x = p.x;
    int y = p.y;
    float cosa = 1.0f*x/sqrt( 1.0f * (x*x + y*y));
    float a = 180/PI * acos (cosa);
    if (y < 0)
        a = -a;
    return floor(a);
}
static inline int getDistance (Point p)
{
    return sqrt (p.x*p.x + p.y*p.y);
}

#endif
