/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "motor_control.h"
static int X = 10;
static int Y = 10;
static int A = 0;
static int VL = 0, VR = 0;
static int DIS = 50;

static int segdis = 2;


static double getrota_time (int v,int angle )
{
    //float a = (angle + 22.5) * 144.0f/141;
    float a = angle;
    //mylogfd (-1, "a=%f, angle=%d v=%d\n", a,angle, v);
    double t = a/360*DD*PI/v * ROTA_TIME;
    return t * 1000000;
}
static double getrun_time (int v, int distance)
{
    double t = distance * 1000000.0f/v;
    return t;
}

static int kb_rotaL(int angle)
{
    mylogfd (1, " left rota\t");
    double t = getrota_time (X, angle);
    sendA(-X,X,10,1);
    mylogfd (-1, "usleep t=%f\t", t);
    usleep(t);
    mylogfd (-1, "stop\n", t);
    sendAA(0,0,0);
    return 0;
}
static int kb_rotaR(int angle)
{
    mylogfd (1, " right rota\t");
    double t = getrota_time (X, angle);
    sendA(X,-X,10,1);
    mylogfd (-1, "usleep t=%f\t", t);
    usleep(t);
    mylogfd (-1, "stop\n", t);
    sendAA(0,0,0);
    return 0;
}
static int kb_run_forward (int v, int distance)
{
    mylogfd (1, "run forward %d\t", distance);
    sendAA(v,v,A);
    double t = getrun_time (v, distance);
    mylogfd (-1, "usleep t=%f\t", t);
    usleep(t);
    mylogfd (-1, "stop\n");
    sendAA(0,0,0);
    return 0;
}
static Trace *trace_plan_line ( Point startP, int startD,
                                Point endP, int endD)
{
    Point pp;
    pp.x = endP.x - startP.x;
    pp.y = endP.y - startP.y;
    int angle = getAngle (pp);
    int num = 4;
    //if ( endD -angle <= ANGLESEG/2 && endD -angle >= -ANGLESEG/2) num = 3;
    Trace *tr = newTrace (num);
    tr->ps[0] = startP;
    tr->direction[0] = startD;
    tr->ps[1] = startP;

    tr->direction[1] = angle;
    tr->ps[2] = endP;
    tr->direction[2] = angle;
    tr->ps[num-1] = endP;
    tr->direction[num-1] = endD;
    return tr;
};

static int kb_forward1  (int argc, char **argv) //t
{
    return kb_run_forward (X, DIS*1);
}
static int kb_forward2  (int argc, char **argv) //y
{
    return kb_run_forward (X, DIS*2);
}
static int kb_forward3  (int argc, char **argv) //u
{
    return kb_run_forward (X, DIS*3);
}
static int kb_forward4  (int argc, char **argv) //i
{
    return kb_run_forward (X, DIS*4);
}
static int kb_forward5  (int argc, char **argv) //o
{
    return kb_run_forward (X, DIS*5);
}
static int kb_forward6  (int argc, char **argv) //p
{
    return kb_run_forward (X, DIS*6);
}
static int kb_forward7  (int argc, char **argv) //[
{
    return kb_run_forward (X, DIS*7);
}
static int kb_forward8  (int argc, char **argv) //]
{
    return kb_run_forward (X, DIS*8);
}

static int kb_forward  (int argc, char **argv) //w
{
    mylogfd (1, "forward\n");
    VL = X;
    VR = Y;
    sendAA(VL,VR,A);
    return 0;
}
static int kb_left     (int argc, char **argv) //a
{
    mylogfd (1, "left\n");
    VL = X/2;
    VR = Y;
    sendAA(VL,VR,A);
    return 0;
}
static int kb_right    (int argc, char **argv) //d
{
    mylogfd (1, "right\n");
    VL = X;
    VR = Y/2;
    sendAA(VL,VR,A);
    return 0;
}
static int kb_back     (int argc, char **argv) //s
{
    mylogfd (1, "back\t");
    VL = -X;
    VR = -Y;
    sendAA(VL,VR,A);
    return 0;
}
static int kb_speedup  (int argc, char **argv) //=
{
    mylogfd (1, "speedup\n");
    int inc = 5;
    if (VL < 0)
        VL -=inc;
    else
        VL +=5;
    if (VR < 0)
        VR -=inc;
    else
        VR +=5;
    sendAA(VL,VR,0);
    return 0;
}
static int kb_slowdown (int argc, char **argv) //-
{
    mylogfd (1, "slowdown\n");
    int inc = 5;
    if (VL < 0)
        VL +=inc;
    else
        VL -= 5;
    if (VR < 0)
        VR +=inc;
    else
        VR -= 5;
    sendAA(VL,VR,0);
    return 0;
}
static int kb_adjustL1 (int argc, char **argv) //9
{
    mylogfd (1, "adjustL1\n");
    double t = getrun_time (VL, segdis);
    mylogfd (1, "usleep t=%f\t", t);
    sendAA(VL+5,VR,0);
    usleep(t);
    mylogfd (1, "sleep over stop\n");
    sendAA(VL,VR,0);
    return 0;
}
static int kb_adjustL2 (int argc, char **argv) //0
{
    mylogfd (1, "adjustL2\n");
    double t = getrun_time (VL, segdis);
    mylogfd (1, "usleep t=%f\t", t);
    sendAA(VL+8,VR,0);
    usleep(t);
    mylogfd (1, "sleep over stop\n");
    sendAA(VL,VR,0);
    return 0;
}

static int kb_rotaOL1  (int argc, char **argv) //1
{
    return kb_rotaL (180);
}
static int kb_rotaOL2  (int argc, char **argv) //2
{
    return kb_rotaL (360);
}
static int kb_rotaOL3  (int argc, char **argv) //3
{
    return kb_rotaL (180*3);
}
static int kb_rotaOL4  (int argc, char **argv) //4
{
    return kb_rotaL (180*4);
}
static int kb_rotaOL5  (int argc, char **argv) //5
{
    return kb_rotaL (180*5);
}
static int kb_rotaOL6  (int argc, char **argv) //6
{
    return kb_rotaL (180*6);
}
static int kb_rotaOL7  (int argc, char **argv) //7
{
    return kb_rotaL (180*7);
}
static int kb_rotaOL8  (int argc, char **argv) //8
{
    return kb_rotaL (180*8);
}

static int kb_rotaOL   (int argc, char **argv) //z
{
    return kb_rotaL (30);
}
static int kb_rotaOR   (int argc, char **argv) //x
{
    return kb_rotaR (30);
}
static int kb_rotaLL   (int argc, char **argv) //c
{
    mylogfd (1, "left turn left\t");
    sendAA(0,X,A);
    return 0;
}
static int kb_rotaLR   (int argc, char **argv) //v
{
    mylogfd (1, "left turn right\t");
    sendAA(0,-X,A);
    return 0;
}
static int kb_rotaRL   (int argc, char **argv) //b
{
    mylogfd (1, "right turn left\t");
    sendAA(-X,0,A);
    return 0;
}
static int kb_rotaRR   (int argc, char **argv) //n
{
    mylogfd (1, "right turn right\t");
    sendAA(X,0,A);
    return 0;
}

static int kb_rotaLL1  (int argc, char **argv) //f
{
    mylogfd (1, "left turn left\t");
    sendAA(5,15,A);
    return 0;
}
static int kb_rotaLL2  (int argc, char **argv) //g
{
    mylogfd (1, "left turn left\t");
    sendAA(7,15,A);
    return 0;
}
static int kb_rotaLL3  (int argc, char **argv) //h
{
    mylogfd (1, "left turn left\t");
    sendAA(10,15,A);
    return 0;
}
static int kb_rotaLL4  (int argc, char **argv) //j
{
    mylogfd (1, "left turn left\t");
    sendAA(8,20,0);
    return 0;
}
static int kb_rotaLL5  (int argc, char **argv) //k
{
    mylogfd (1, "left turn left\t");
    sendAA(10,20,0);
    return 0;
}
static int kb_rotaLL6  (int argc, char **argv) //l
{
    mylogfd (1, "left turn left\t");
    sendAA(15,20,0);
    return 0;
}
static int kb_rotaLL7  (int argc, char **argv) //;
{
    mylogfd (1, "left turn left\t");
    sendAA(15,25,0);
    return 0;
}

static int kb_run      (int argc, char **argv) //e
{
    mylogfd (1, "run\t");
    Point start, end;
    start.x = start.y = 0;
    end.x = 100;
    end.y = 0;

    Task t;
    t.type = TASK_RUN;
    t.trace = trace_plan_line (start, 0, end, 0);
    sendTask(t);

    return 0;
}
static int kb_rotation (int argc, char **argv) //r
{
    int angle = 360;
    int slice = 1;
    mylogfd (1, "kb rotation angle=%d, slice=%d\n");
    Task t;
    t.type = TASK_ROTA;
    t.rotslice = slice;
    t.rotangle = angle;
    sendTask(t);
    return 0;
}

static int kb_stop     (int argc, char **argv) //space
{
    mylogfd (1, "stop\n");
    sendSTOP();
    return 0;
}
static int kb_help     (int argc, char **argv) //h
{
    mylogfd (1, "help\t");
    return 0;
}


static struct cmd kbtab[] =  {
    { "w",      kb_forward},
    { "a",      kb_left},
    { "d",      kb_right},
    { "s",      kb_back},
    { "=",      kb_speedup},
    { "-",      kb_slowdown},

    { "t",      kb_forward1},
    { "y",      kb_forward2},
    { "u",      kb_forward3},
    { "i",      kb_forward4},
    { "o",      kb_forward5},
    { "p",      kb_forward6},
    { "[",      kb_forward7},
    { "]",      kb_forward8},

    { "1",      kb_rotaOL1}, // 逆 原地旋转180度
    { "2",      kb_rotaOL2},
    { "3",      kb_rotaOL3},
    { "4",      kb_rotaOL4},
    { "5",      kb_rotaOL5},
    { "6",      kb_rotaOL6},
    { "7",      kb_rotaOL7},
    { "8",      kb_rotaOL8},

    { "z",      kb_rotaOL}, //逆 原地旋转30度
    { "x",      kb_rotaOR}, //顺 原地旋转30度
    { "c",      kb_rotaLL}, //以左轮为轴
    { "v",      kb_rotaLR}, //以左轮为轴
    { "b",      kb_rotaRL}, //以右轮为轴
    { "n",      kb_rotaRR}, //以右轮为轴

    { "f",      kb_rotaLL1}, //以左轮为轴
    { "g",      kb_rotaLL2}, //以左轮为轴
    { "h",      kb_rotaLL3}, //以左轮为轴
    { "j",      kb_rotaLL4}, //以左轮为轴
    { "k",      kb_rotaLL5}, //以左轮为轴
    { "l",      kb_rotaLL6}, //以左轮为轴
    { ";",      kb_rotaLL7}, //以左轮为轴

    { "9",      kb_adjustL1},
    { "0",      kb_adjustL2},
    { "e",      kb_run},
    { "r",      kb_rotation},

    { " ",      kb_stop},
    { "h",      kb_help},
    { 0 }
};

void kb_exit (int signo)
{
    motor_exit();
    signal (signo, SIG_DFL);
    raise (signo);
}
int kbctrl (int argc, char **argv)
{
    struct termios tio;
    struct cmd *c;
    char buf[2];
    int ret = 0;
    buf[1] = '\0';
    tty_set(&tio);
    signal (SIGINT, kb_exit);
    while (1) {
        ret = read (0, buf, 1);
        if (buf[0] == '\n' || buf[0] == '\r') continue;
        buf[1] = '\0';
        mylogfd (1, "cmd>>[%c]", buf[0]);
        if (buf[0] == 'q') break;

        c = getcmd (buf, kbtab);
        if (c == NULL)
            mylogfd (1, "Unknown command \"%s\"\n", buf);
        else
            c->handler (0, NULL);
        //fflush(stdout);
    }
    motor_exit();
    return 0;
}
