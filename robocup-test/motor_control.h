/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H


#define TIMESLICE 1
#define CTRL_INTERVAL 0.01

#define ADDV 10
#define ROTV 10
#define STARTV 10
#define SEG 5
#define LITV 5
#define MINV 30
#define MIN_SPEED 10
#define MOTO_SHOOT_DIS 25

#define VVEC_AMP 10
#define HALFDISWHEELS 30/2

#define PI 3.1415926
#define ROTA_TIME 1

#include "common.h"

typedef enum {TASK_STOP, TASK_ROTA, TASK_CATCH_BALL, TASK_WITH_BALL, TASK_RUN, TASK_DOT_SHOOT
             } TASKTYPE;

//void getSpeed (int *left, int *right);

typedef struct Task {
    TASKTYPE type;
    Trace *trace;//运动轨迹
    int rotangle;//旋转角度+-360， +为逆时针，-为顺时针
    int rotslice;//>0为分片为分片大小，0为不分片
    int rota_speed;
    int maxa;
} Task;
typedef Task task;

struct task_seg {
    TASKTYPE type;
    Point start, end;
    double lspeed, rspeed;
    double dur_time;
    int rotangle;//旋转角度+-360， +为逆时针，-为顺时针
};

//static inline void cleanup();



void motor_init();
void motor_exit();
void motor_thread_create();

void sendAA(int vleft, int vright, int a);
void sendA(int left,int right,int a, int type);

void sendSTOP();
void sendTask(Task t);
void sendLeft();
void sendRight();
void sendStopAdjust();
int checkDone();

int kbctrl(int argc, char **argv);

#endif

