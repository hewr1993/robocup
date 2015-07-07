/*: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "motor_control.h"

#include "cmd.h"
#include "tty.h"
#include "ctrl.h"
#include "sched.h"

#include <stdlib.h>
#include <sys/poll.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>



#define CATCH_BALL_L 10
#define CATCH_BALL_R 20
#define __DIS 5
static const int MOTOFD = -3;
static void stopTask_init(task t);
static void rotaTask_init(task t);
static void runTask_init(task t);
static double compute_t(int rota,double r,double speed);
static void compute_c(Point *answer,Point a,Point b,int d_a,int d_b);

int fd;
char buf[16];
char rbuf[16];
double vleft=0, vright=0;
double preleft = 0;
double preright = 0;
struct termios tio;
task current;
struct task_seg *task_segList;
int task_segIdx = 0, task_segNum = 0;
int maxa = 0;

int job_done = 1;

pthread_mutex_t task_mutex;
pthread_mutex_t stop_mutex;
int mutex_flag = 0;
int left_flag = 0;
int right_flag = 0;

/*
 *cleanup
 */
static inline void cleanup()
{
    sendSTOP();
    tcsetattr(0, TCSANOW, &tio);
    //pthread_mutex_destroy(&task_mutex);
}
/*
 *send msg to motor
 */
static void msend(void *buf, int n)
{
    int t;
    if ((t = write(fd, buf, n)) != n) {
        if (t == -1)
            mylogfd(2,"[moto]msend: write\n");
        else
            mylogfd(2,"[moto]msend: write error\n");
        exit(-1);
    }
}
/*
 *read msg from motor
 */
/*
static int mread(void *buf){
	int t;
	t = read(fd, buf, sizeof(buf));
	if (t == -1)
		mylogfd(2,"[moto]read: read\n");
	return t;
}
*/
static void compute_a(double *left,double *right,double time,int i,double rate)
{
    if (fabs(*left - vleft) > fabs(*right - vright)) {
        if (fabs(*left - vleft)/time > maxa) {
            if (i == 0) {
                time = (sqrt((*left)*(*left) + 2*maxa*SEGDIS)-(*left))/maxa;
                if (rate >= 1) {
                    *left = vleft + maxa*time;
                    *right = *left/rate;
                } else {
                    *right = vright + maxa*time;
                    *left = *right*rate;
                }
            } else {
                if ((*left)*(*left) - 2*maxa*SEGDIS < 0) {
                    *left = 0;
                    *right = 0;
                } else {
                    time = ((*left) - sqrt((*left)*(*left) - 2*maxa*SEGDIS))/maxa;
                    if (rate >= 1) {
                        *left = vleft -maxa*time;
                        *right = *left/rate;
                    } else {
                        *right = vright - maxa*time;
                        *left = *right*rate;
                    }
                }
            }
        }
        vleft = *left;
        vright = *right;
        return;
    } else {
        if (fabs(*right - vright)/time > maxa) {
            if (i == 0) {
                if (rate >= 1) {
                    *left = vleft + maxa*time;
                    *right = *left/rate;
                } else {
                    *right = vright + maxa*time;
                    *left = *right*rate;
                }
            } else {
                if (rate >= 1) {
                    *left = vleft -maxa*time;
                    *right = *left/rate;
                } else {
                    *right = vright - maxa*time;
                    *left = *right*rate;
                }
            }
        }
    }
    vleft = *left;
    vright = *right;
    return;
}

/*
 *compute the rota time
 */
static double compute_t(int rota,double r,double speed)
{
    return fabs(12.0f/13 *rota / 180.0f * PI * r / speed);
}
/*
 *compute the center of a circle
 */
static void compute_c(Point *answer,Point a,Point b,int d_a,int d_b)
{
    double delta1 = tan((d_a)*PI/180-PI/2);
    double delta2 = tan((d_b)*PI/180-PI/2);
    answer->x = ((a.y - b.y) - delta1*a.x + delta2*b.x)/(delta2-delta1);
    answer->y = delta1*(answer->x - a.x) + a.y;
    return;
}

/*
 *if task_type == TASK_STOP,setup task_segList
 */
static void stopTask_init(task t)
{
    //mylogfd(-1,"[Control]/motor/stopTask_init\n");
    task_segList[0].type = TASK_STOP;
    task_segList[0].start.x = 0;
    task_segList[0].start.y = 0;
    task_segList[0].end.x = 0;
    task_segList[0].end.y = 0;
    task_segList[0].lspeed = 0;
    vleft = 0;
    task_segList[0].rspeed = 0;
    vright = 0;
    task_segList[0].rotangle = 0;
    task_segList[0].dur_time = 0;
    return;
}
/*
 *if task_type == TASK_ROTA,setup task_segList
 */
static void rotaTask_init(task t)
{
    //mylogfd(-1,"[Control]/motor/rotaTask_init\n");
    int i;
    //不分片
    if (t.rotslice == 0) {
        task_segList[0].type = TASK_ROTA;
        task_segList[0].start.x = 0;
        task_segList[0].start.y = 0;
        task_segList[0].end.x = 0;
        task_segList[0].end.y = 0;
        if (t.rotangle > 0) {
            task_segList[0].lspeed = -t.rota_speed;
            task_segList[0].rspeed = t.rota_speed;
        } else {
            task_segList[0].lspeed = t.rota_speed;
            task_segList[0].rspeed = -t.rota_speed;
        }
        task_segList[0].rotangle = t.rotangle;
        task_segList[0].dur_time = compute_t(task_segList[0].rotangle,
                                             DD/2,task_segList[0].lspeed);
        return;
    } else {   //分片
        for (i = 0; i < task_segNum - 1; i++) {
            task_segList[i].type = TASK_ROTA;
            if (t.rotangle > 0) {
                task_segList[i].lspeed = -t.rota_speed;
                task_segList[i].rspeed = t.rota_speed;
            } else {
                task_segList[i].lspeed = t.rota_speed;
                task_segList[i].rspeed = -t.rota_speed;
            }
            task_segList[i].rotangle = t.rotslice;
            task_segList[i].dur_time = compute_t(task_segList[i].rotangle,
                                                 DD/2,task_segList[i].lspeed);
        }
        task_segList[i].type = TASK_ROTA;
        if (t.rotangle > 0) {
            task_segList[i].lspeed = -t.rota_speed;
            task_segList[i].rspeed = t.rota_speed;
        } else {
            task_segList[i].lspeed = t.rota_speed;
            task_segList[i].rspeed = -t.rota_speed;
        }
        task_segList[i].rotangle = t.rotangle - task_segNum * t.rotslice;
        task_segList[i].dur_time = compute_t(task_segList[i].rotangle,
                                             DD/2,fabs(task_segList[i].lspeed));
    }
    return;
}
static void withballTask_init(task t)
{
    int i;
    double rr = (CATCH_BALL_R+CATCH_BALL_L)*DD/(CATCH_BALL_R-CATCH_BALL_L)/2 + DD/2;
    //不分片
    if (t.rotslice == 0) {
        task_segList[0].type = TASK_WITH_BALL;
        task_segList[0].start.x = 0;
        task_segList[0].start.y = 0;
        task_segList[0].end.x = 0;
        task_segList[0].end.y = 0;
        if (t.rotangle > 0) {
            task_segList[0].lspeed = CATCH_BALL_L;
            task_segList[0].rspeed = CATCH_BALL_R;
            task_segList[0].rotangle = t.rotangle;
            task_segList[0].dur_time = compute_t(task_segList[0].rotangle,
                                                 rr,task_segList[0].rspeed);
        } else {
            task_segList[0].lspeed = CATCH_BALL_R;
            task_segList[0].rspeed = CATCH_BALL_L;
            task_segList[0].rotangle = t.rotangle;
            task_segList[0].dur_time = compute_t(task_segList[0].rotangle,
                                                 rr,task_segList[0].lspeed);
        }
        return;
    } else { //分片
        task_segList[0].type = TASK_WITH_BALL;
        task_segList[0].start.x = 0;
        task_segList[0].start.y = 0;
        task_segList[0].end.x = 0;
        task_segList[0].end.y = 0;
        task_segList[0].lspeed = ROTV;
        task_segList[0].rspeed = ROTV;
        task_segList[0].rotangle = 0;
        task_segList[0].dur_time = fabs(__DIS/task_segList[0].lspeed);
        for (i = 1; i < task_segNum - 1; i++) {
            task_segList[i].type = TASK_WITH_BALL;
            if (t.rotangle > 0)  {
                task_segList[i].lspeed = CATCH_BALL_L;
                task_segList[i].rspeed = CATCH_BALL_R;
                task_segList[i].rotangle = t.rotslice;
                task_segList[i].dur_time = compute_t(task_segList[i].rotangle,
                                                     rr,task_segList[i].rspeed);
            } else {
                task_segList[i].lspeed = CATCH_BALL_R;
                task_segList[i].rspeed = CATCH_BALL_L;
                task_segList[i].rotangle = t.rotslice;
                task_segList[i].dur_time = compute_t(task_segList[i].rotangle,
                                                     rr,task_segList[i].lspeed);
            }
        }
        task_segList[i].type = TASK_WITH_BALL;
        if (t.rotangle > 0) {
            task_segList[i].lspeed = CATCH_BALL_L;
            task_segList[i].rspeed = CATCH_BALL_R;
            task_segList[i].rotangle = t.rotangle - task_segNum * t.rotslice;
            task_segList[i].dur_time = compute_t(task_segList[i].rotangle,
                                                 rr,task_segList[i].rspeed);
        } else {
            task_segList[i].lspeed = CATCH_BALL_R;
            task_segList[i].rspeed = CATCH_BALL_L;
            task_segList[i].rotangle = t.rotangle - task_segNum * t.rotslice;
            task_segList[i].dur_time = compute_t(task_segList[i].rotangle,
                                                 rr,task_segList[i].lspeed);
        }
    }
    return;
}

static void dotshootTask_init(task t)
{
    mylogfd (MOTOFD, "dotshootTask_init\n");
    int i = 0;
    double rate = 1;
    int delta = 0;
    double r = 0;
    maxa = t.maxa;
    for (i = 0; i < task_segNum - 2; i++) {
        task_segList[i].type = TASK_RUN;
        task_segList[i].start.x = t.trace->ps[i].x;
        task_segList[i].start.y = t.trace->ps[i].y;
        task_segList[i].end.x = t.trace->ps[i+1].x;
        task_segList[i].end.y = t.trace->ps[i+1].y;
        if ((task_segList[i].start.x == task_segList[i].end.x)
                &&(task_segList[i].start.y == task_segList[i].end.y)) {
            task_segList[i].type = TASK_ROTA;
            int rot = t.trace->direction[i+1]-t.trace->direction[i];
            if (rot > 0) {
                task_segList[i].lspeed = 2*-ROTV;
                task_segList[i].rspeed = 2*ROTV;
            } else {
                task_segList[i].lspeed = 2*ROTV;
                task_segList[i].rspeed = 2*-ROTV;
            }
            task_segList[i].rotangle = rot;
            task_segList[i].dur_time = compute_t(task_segList[i].rotangle,
                                                 DD/2,task_segList[i].lspeed);
            continue;
        }
        if (t.trace->direction[i] == t.trace->direction[i+1]) {
            if (t.trace->inverse[i] == 1) { //反向
                if (vleft != vright) {
                    if (fabs(vleft) > fabs(vright)) {
                        if (vleft < 0)
                            vright = vleft;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    } else {
                        if (vright < 0)
                            vleft = vright;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    }
                }
                task_segList[i].lspeed = vleft - 2*ADDV;
                //vleft = task_segList[i].lspeed;
                task_segList[i].rspeed = task_segList[i].lspeed;
                //vright = task_segList[i].rspeed;
            } else { //正向
                if (vleft != vright) {
                    if (fabs(vleft) > fabs(vright)) {
                        if (vleft > 0)
                            vright = vleft;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    } else {
                        if (vright > 0)
                            vleft = vright;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    }
                }
                task_segList[i].lspeed = vleft + ADDV;
                //vleft = task_segList[i].lspeed;
                task_segList[i].rspeed = task_segList[i].lspeed;
                //vright = task_segList[i].rspeed;
            }
            double d = sqrt((t.trace->ps[i].x - t.trace->ps[i+1].x)
                            *(t.trace->ps[i].x - t.trace->ps[i+1].x)
                            +(t.trace->ps[i].y - t.trace->ps[i+1].y)
                            *(t.trace->ps[i].y - t.trace->ps[i+1].y));
            task_segList[i].dur_time = fabs(d / task_segList[i].lspeed);
            compute_a(&task_segList[i].lspeed,&task_segList[i].rspeed,
                      task_segList[i].dur_time,t.trace->inverse[i],1);
            if (task_segList[i].lspeed > MAXV)
                task_segList[i].lspeed = MAXV;
            if (task_segList[i].lspeed < -MAXV)
                task_segList[i].lspeed = -MAXV;
            if (task_segList[i].rspeed > MAXV)
                task_segList[i].rspeed = MAXV;
            if (task_segList[i].rspeed < -MAXV)
                task_segList[i].rspeed = -MAXV;
            task_segList[i].dur_time = fabs(d / task_segList[i].lspeed);
        } else { //圆弧
            //计算圆心

            Point *circle;
            circle = (Point*)malloc(sizeof(struct _Point));
            compute_c(circle,t.trace->ps[i],t.trace->ps[i+1],
                      t.trace->direction[i],t.trace->direction[i+1]);
            //计算半径
            r = t.trace->r[i];
            //速度比
            rate = (r+DD/2)/(r-DD/2);
            mylogfd(MOTOFD,"get in arc r=%f rate=%f\n",r,rate);
            if (t.trace->direction[i] < t.trace->direction[i+1])
                rate = 1/rate;
            //右旋
            if (rate > 1) {
                if (t.trace->inverse[i] == 0) {
                    if (vleft < 0)
                        vleft = -vleft;
                    if (vleft == vright)
                        vright = vleft/rate;
                    task_segList[i].lspeed = vleft + ADDV;
                    //vleft = task_segList[i].lspeed;
                    task_segList[i].rspeed = task_segList[i].lspeed/rate;
                    //vright = task_segList[i].rspeed;
                } else {
                    if (vleft > 0)
                        vleft = -vleft;
                    if (vleft == vright)
                        vright = vleft/rate;
                    task_segList[i].lspeed = vleft - ADDV;
                    //vleft = task_segList[i].lspeed;
                    task_segList[i].rspeed = task_segList[i].lspeed/rate;
                    //vright = task_segList[i].rspeed;
                }
                delta = abs(t.trace->direction[i] - t.trace->direction[i+1]);
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].lspeed) - 0.07;
                compute_a(&task_segList[i].lspeed,&task_segList[i].rspeed,
                          task_segList[i].dur_time,t.trace->inverse[i],rate);
                if (task_segList[i].lspeed > MAXV) {
                    task_segList[i].lspeed = MAXV;
                    task_segList[i].rspeed = MAXV/rate;
                }
                if (task_segList[i].lspeed < -MAXV)  {
                    task_segList[i].lspeed = -MAXV;
                    task_segList[i].rspeed = -MAXV/rate;
                }
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].lspeed);
                task_segList[i].dur_time = task_segList[i].dur_time * 13.0f / 12 - 0.07;
            }

            else { //左旋
                if (t.trace->inverse[i] == 0) {
                    if (vleft < 0)
                        vleft = -vleft;
                    if (vleft == vright)
                        vleft = vright*rate;
                    task_segList[i].rspeed = vright + ADDV;
                    //vright = task_segList[i].rspeed;
                    task_segList[i].lspeed = task_segList[i].rspeed*rate;
                    //vleft = task_segList[i].lspeed;
                } else  {
                    if (vleft > 0)
                        vleft = -vleft;
                    if (vleft == vright)
                        vleft = vright*rate;
                    task_segList[i].rspeed = vright - ADDV;
                    //vright = task_segList[i].rspeed;
                    task_segList[i].lspeed = task_segList[i].rspeed*rate;
                    //vleft = task_segList[i].lspeed;
                }
                delta = abs(t.trace->direction[i] - t.trace->direction[i+1]);
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].rspeed);
                compute_a(&task_segList[i].lspeed,&task_segList[i].rspeed,
                          task_segList[i].dur_time,t.trace->inverse[i],rate);
                if (task_segList[i].rspeed > MAXV)  {
                    task_segList[i].rspeed = MAXV;
                    task_segList[i].lspeed = MAXV*rate;
                }
                if (task_segList[i].rspeed < -MAXV) {
                    task_segList[i].rspeed = -MAXV;
                    task_segList[i].lspeed = -MAXV*rate;
                }
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].rspeed);
                task_segList[i].dur_time = task_segList[i].dur_time * 13.0f / 12 - 0.07;
            }
        }
        task_segList[i].rotangle = 0;
        //mylogfd(-1,"<%d,%d>",(int)task_segList[i].lspeed,(int)task_segList[i].rspeed);
    }
    task_segList[i].type = TASK_RUN;
    task_segList[i].start.x = 0;
    task_segList[i].start.y = 0;
    task_segList[i].end.x = 0;
    task_segList[i].end.y = 0;
    task_segList[i].lspeed = task_segList[i-1].lspeed;
    //vleft = 0;
    task_segList[i].rspeed = task_segList[i-1].rspeed;
    //vright = 0;
    task_segList[i].rotangle = 0;
    task_segList[i].dur_time = MOTO_SHOOT_DIS/task_segList[i].lspeed;
    i++;
    task_segList[i].type = TASK_STOP;
    task_segList[i].start.x = 0;
    task_segList[i].start.y = 0;
    task_segList[i].end.x = 0;
    task_segList[i].end.y = 0;
    task_segList[i].lspeed = task_segList[i-1].lspeed;
    vleft = 0;
    task_segList[i].rspeed = task_segList[i-1].rspeed;
    vright = 0;
    task_segList[i].rotangle = 0;
    task_segList[i].dur_time = 0;
    mylogfd (MOTOFD, "dotshootTask_init over\n");
}

/*
 *if task_type == TASK_RUN,setup task_segList;
 */
static void runTask_init(task t)
{
    mylogfd(-1,"[Control]/motor/runTask_init\n");
    int i = 0;;
    double rate = 1;
    int delta = 0;
    double r = 0 ;
    maxa = t.maxa;
    Point *circle;
    circle = (Point*)malloc(sizeof(struct _Point));
    for (i = 0; i < task_segNum; i++) {
        if (fabs((vleft + vright)/2 * (vleft + vright)/2)
                >=2*maxa*(task_segNum-i-1)*SEGDIS) {
            break;
        }
        task_segList[i].type = TASK_RUN;
        task_segList[i].start.x = t.trace->ps[i].x;
        task_segList[i].start.y = t.trace->ps[i].y;
        task_segList[i].end.x = t.trace->ps[i+1].x;
        task_segList[i].end.y = t.trace->ps[i+1].y;
        if ((task_segList[i].start.x == task_segList[i].end.x)
                &&(task_segList[i].start.y == task_segList[i].end.y)) {
            task_segList[i].type = TASK_ROTA;
            int rot = t.trace->direction[i+1]-t.trace->direction[i];
            if (rot > 0) {
                task_segList[i].lspeed = -ROTV;
                task_segList[i].rspeed = ROTV;
            } else {
                task_segList[i].lspeed = ROTV;
                task_segList[i].rspeed = -ROTV;
            }
            task_segList[i].rotangle = rot;
            task_segList[i].dur_time = compute_t(task_segList[i].rotangle,
                                                 DD/2,task_segList[i].lspeed);
            continue;
        }
        //直线
        if (t.trace->direction[i] == t.trace->direction[i+1]) {
            if (t.trace->inverse[i] == 1) { //反向
                if (vleft != vright) {
                    if (fabs(vleft) > fabs(vright)) {
                        if (vleft < 0)
                            vright = vleft;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    } else {
                        if (vright < 0)
                            vleft = vright;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    }
                }
                task_segList[i].lspeed = vleft - ADDV;
                //vleft = task_segList[i].lspeed;
                task_segList[i].rspeed = vright - ADDV;
                //vright = task_segList[i].rspeed;
            } else { //正向
                if (vleft != vright) {
                    if (fabs(vleft) > fabs(vright)) {
                        if (vleft > 0)
                            vright = vleft;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    } else {
                        if (vright > 0)
                            vleft = vright;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    }
                }
                task_segList[i].lspeed = vleft + ADDV;
                //vleft = task_segList[i].lspeed;
                task_segList[i].rspeed = vright + ADDV;
                //vright = task_segList[i].rspeed;
            }
            if (task_segList[i].lspeed > MAXV)
                task_segList[i].lspeed = MAXV;
            if (task_segList[i].lspeed < -MAXV)
                task_segList[i].lspeed = -MAXV;
            if (task_segList[i].rspeed > MAXV)
                task_segList[i].rspeed = MAXV;
            if (task_segList[i].rspeed < -MAXV)
                task_segList[i].rspeed = -MAXV;

            double d = sqrt((t.trace->ps[i].x - t.trace->ps[i+1].x)
                            *(t.trace->ps[i].x - t.trace->ps[i+1].x)
                            +(t.trace->ps[i].y - t.trace->ps[i+1].y)
                            *(t.trace->ps[i].y - t.trace->ps[i+1].y));
            task_segList[i].dur_time = fabs(d / task_segList[i].lspeed);
            compute_a(&task_segList[i].lspeed,&task_segList[i].rspeed,
                      task_segList[i].dur_time,t.trace->inverse[i],1);
            task_segList[i].dur_time = fabs(d / task_segList[i].lspeed);
        }

        else { //圆弧
            //计算圆心
            compute_c(circle,t.trace->ps[i],t.trace->ps[i+1],
                      t.trace->direction[i],t.trace->direction[i+1]);
            //计算半径
            r = t.trace->r[i];
            //速度比
            rate = (r+DD/2)/(r-DD/2);
            if (t.trace->direction[i] < t.trace->direction[i+1])
                rate = 1/rate;
            //右旋
            if (rate > 1) {
                if (t.trace->inverse[i] == 0) {
                    if (vleft == vright)
                        vright = vleft/rate;
                    task_segList[i].lspeed = vleft + ADDV;
                    //vleft = task_segList[i].lspeed;
                    task_segList[i].rspeed = task_segList[i].lspeed/rate;
                    //vright = task_segList[i].rspeed;
                } else {
                    if (vleft == vright)
                        vright = vleft/rate;
                    task_segList[i].lspeed = vleft - ADDV;
                    //vleft = task_segList[i].lspeed;
                    task_segList[i].rspeed = task_segList[i].lspeed/rate;
                    //vright = task_segList[i].rspeed;
                }
                delta = abs(t.trace->direction[i] - t.trace->direction[i+1]);
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].lspeed);
                compute_a(&task_segList[i].lspeed,&task_segList[i].rspeed,
                          task_segList[i].dur_time,t.trace->inverse[i],rate);
                if (task_segList[i].lspeed > MAXV) {
                    task_segList[i].lspeed = MAXV;
                    task_segList[i].rspeed = MAXV/rate;
                }
                if (task_segList[i].lspeed < -MAXV)  {
                    task_segList[i].lspeed = -MAXV;
                    task_segList[i].rspeed = -MAXV/rate;
                }
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].lspeed);
            }

            else { //左旋
                if (t.trace->inverse[i] == 0) {
                    if (vleft == vright)
                        vleft = vright*rate;
                    task_segList[i].rspeed = vright + ADDV;
                    //vright = task_segList[i].rspeed;
                    task_segList[i].lspeed = task_segList[i].rspeed*rate;
                    //vleft = task_segList[i].lspeed;
                } else  {
                    if (vleft == vright)
                        vleft = vright*rate;
                    task_segList[i].rspeed = vright - ADDV;
                    //vright = task_segList[i].rspeed;
                    task_segList[i].lspeed = task_segList[i].rspeed*rate;
                    //vleft = task_segList[i].lspeed;
                }
                delta = abs(t.trace->direction[i] - t.trace->direction[i+1]);
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].rspeed);
                compute_a(&task_segList[i].lspeed,&task_segList[i].rspeed,
                          task_segList[i].dur_time,t.trace->inverse[i],rate);
                if (task_segList[i].rspeed > MAXV)  {
                    task_segList[i].rspeed = MAXV;
                    task_segList[i].lspeed = MAXV*rate;
                }
                if (task_segList[i].rspeed < -MAXV) {
                    task_segList[i].rspeed = -MAXV;
                    task_segList[i].lspeed = -MAXV*rate;
                }
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].rspeed);
            }
        }
        task_segList[i].rotangle = 0;
        //mylogfd(1,"<%d,%d>",(int)task_segList[i].lspeed,(int)task_segList[i].rspeed);
    }
    for (; i < task_segNum-1; i++) {
        task_segList[i].type = TASK_RUN;
        task_segList[i].start.x = t.trace->ps[i].x;
        task_segList[i].start.y = t.trace->ps[i].y;
        task_segList[i].end.x = t.trace->ps[i+1].x;
        task_segList[i].end.y = t.trace->ps[i+1].y;
        //直线
        if (t.trace->direction[i] == t.trace->direction[i+1]) {
            if (t.trace->inverse[i] == 1) { //反向
                if (vleft != vright) {
                    if (fabs(vleft) > fabs(vright)) {
                        if (vleft < 0)
                            vright = vleft;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    } else {
                        if (vright < 0)
                            vleft = vright;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    }
                }
                task_segList[i].lspeed = vleft + ADDV;
                //vleft = task_segList[i].lspeed;
                task_segList[i].rspeed = task_segList[i].lspeed;
                //vright = task_segList[i].rspeed;
                if (task_segList[i].lspeed > -MIN_SPEED)
                    task_segList[i].lspeed = -MIN_SPEED;
                if (task_segList[i].rspeed > -MIN_SPEED)
                    task_segList[i].rspeed = -MIN_SPEED;
            }

            else { //正向
                if (vleft != vright) {
                    if (fabs(vleft) > fabs(vright)) {
                        if (vleft > 0)
                            vright = vleft;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    } else {
                        if (vright > 0)
                            vleft = vright;
                        else {
                            vleft = 0;
                            vright = 0;
                        }
                    }
                }
                task_segList[i].lspeed = vleft - ADDV;
                //vleft = task_segList[i].lspeed;
                task_segList[i].rspeed = task_segList[i].lspeed;
                //vright = task_segList[i].rspeed;
                if (task_segList[i].lspeed < MIN_SPEED)
                    task_segList[i].lspeed = MIN_SPEED;
                if (task_segList[i].rspeed < MIN_SPEED)
                    task_segList[i].rspeed = MIN_SPEED;
            }
            if (task_segList[i].lspeed > MAXV)
                task_segList[i].lspeed = MAXV;
            if (task_segList[i].lspeed < -MAXV)
                task_segList[i].lspeed = -MAXV;
            if (task_segList[i].rspeed > MAXV)
                task_segList[i].rspeed = MAXV;
            if (task_segList[i].rspeed < -MAXV)
                task_segList[i].rspeed = -MAXV;

            double d = sqrt((t.trace->ps[i].x - t.trace->ps[i+1].x)
                            *(t.trace->ps[i].x - t.trace->ps[i+1].x)
                            +(t.trace->ps[i].y - t.trace->ps[i+1].y)
                            *(t.trace->ps[i].y - t.trace->ps[i+1].y));
            task_segList[i].dur_time = fabs(d / task_segList[i].lspeed);
            compute_a(&task_segList[i].lspeed,&task_segList[i].rspeed,
                      task_segList[i].dur_time,1-t.trace->inverse[i],1);
            task_segList[i].dur_time = fabs(d / task_segList[i].lspeed);
        }

        else { //圆弧
            //计算圆心
            compute_c(circle,t.trace->ps[i],t.trace->ps[i+1],
                      t.trace->direction[i],t.trace->direction[i+1]);
            //计算半径
            r = t.trace->r[i];
            //速度比
            rate = (r+DD/2)/(r-DD/2);
            if (t.trace->direction[i] < t.trace->direction[i+1])
                rate = 1/rate;
            //右旋
            if (rate > 1) {
                if (t.trace->inverse[i] == 0) {
                    if (vleft == vright)
                        vright = vleft/rate;
                    task_segList[i].lspeed = vleft - ADDV;
                    //vleft = task_segList[i].lspeed;
                    task_segList[i].rspeed = task_segList[i].lspeed/rate;
                    //vright = task_segList[i].rspeed;
                } else {
                    if (vleft == vright)
                        vright = vleft/rate;
                    task_segList[i].lspeed = vleft + ADDV;
                    //vleft = task_segList[i].lspeed;
                    task_segList[i].rspeed = task_segList[i].lspeed/rate;
                    //vright = task_segList[i].rspeed;
                }
                delta = abs(t.trace->direction[i] - t.trace->direction[i+1]);
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].lspeed);
                compute_a(&task_segList[i].lspeed,&task_segList[i].rspeed,
                          task_segList[i].dur_time,1-t.trace->inverse[i],rate);
                if (task_segList[i].lspeed < MIN_SPEED) {
                    task_segList[i].lspeed = MIN_SPEED;
                    task_segList[i].rspeed = MIN_SPEED/rate;
                }
                if (task_segList[i].rspeed < MIN_SPEED/rate) {
                    task_segList[i].lspeed = MIN_SPEED;
                    task_segList[i].rspeed = MIN_SPEED/rate;
                }
                if (task_segList[i].lspeed > MAXV) {
                    task_segList[i].lspeed = MAXV;
                    task_segList[i].rspeed = MAXV/rate;
                }
                if (task_segList[i].lspeed < -MAXV)  {
                    task_segList[i].lspeed = -MAXV;
                    task_segList[i].rspeed = -MAXV/rate;
                }
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].lspeed);
            }

            else { //左旋
                if (t.trace->inverse[i] == 0) {
                    if (vleft == vright)
                        vleft = vright*rate;
                    task_segList[i].rspeed = vright - ADDV;
                    //vright = task_segList[i].rspeed;
                    task_segList[i].lspeed = task_segList[i].rspeed*rate;
                    //vleft = task_segList[i].lspeed;
                } else  {
                    if (vleft == vright)
                        vleft = vright*rate;
                    task_segList[i].rspeed = vright - ADDV;
                    //vright = task_segList[i].rspeed;
                    task_segList[i].lspeed = task_segList[i].rspeed*rate;
                    //vleft = task_segList[i].lspeed;
                }
                delta = abs(t.trace->direction[i] - t.trace->direction[i+1]);
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].rspeed);
                compute_a(&task_segList[i].lspeed,&task_segList[i].rspeed,
                          task_segList[i].dur_time,1-t.trace->inverse[i],rate);
                if (task_segList[i].lspeed < MIN_SPEED*rate) {
                    task_segList[i].lspeed = MIN_SPEED*rate;
                    task_segList[i].rspeed = MIN_SPEED;
                }
                if (task_segList[i].rspeed < MIN_SPEED) {
                    task_segList[i].lspeed = MIN_SPEED*rate;
                    task_segList[i].rspeed = MIN_SPEED;
                }
                if (task_segList[i].rspeed > MAXV)  {
                    task_segList[i].rspeed = MAXV;
                    task_segList[i].lspeed = MAXV*rate;
                }
                if (task_segList[i].rspeed < -MAXV) {
                    task_segList[i].rspeed = -MAXV;
                    task_segList[i].lspeed = -MAXV*rate;
                }
                task_segList[i].dur_time = compute_t(delta,(r + DD/2),
                                                     task_segList[i].rspeed);
            }
        }
        task_segList[i].rotangle = 0;
        /* mylogfd(1,"(%d,%d)<%d,%d>{%lf}",circle->x,circle->y,(int)task_segList[i].lspeed,(int)task_segList[i].rspeed,rate); */
    }
    task_segList[i].type = TASK_RUN;
    task_segList[i].start.x = t.trace->ps[i].x;
    task_segList[i].start.y = t.trace->ps[i].y;
    task_segList[i].end.x = t.trace->ps[i+1].x;
    task_segList[i].end.y = t.trace->ps[i+1].y;
    task_segList[i].lspeed = 0;
    task_segList[i].rspeed = 0;
    task_segList[i].dur_time = 0;
    task_segList[i].rotangle = 0;
}

static void catchballTask_init(task t)
{
    //mylogfd(-1,"[Control]/motor/catchballTask_init\n");
    int i;
    for (i = 0; i < task_segNum; i++) {
        task_segList[i].type = TASK_CATCH_BALL;
        task_segList[i].start.x = t.trace->ps[i].x;
        task_segList[i].start.y = t.trace->ps[i].y;
        task_segList[i].end.x = t.trace->ps[i+1].x;
        task_segList[i].end.y = t.trace->ps[i+1].y;
        if ((task_segList[i].start.x == task_segList[i].end.x)
                &&(task_segList[i].start.y == task_segList[i].end.y)) {
            task_segList[i].type = TASK_ROTA;
            int rot = t.trace->direction[i]-t.trace->direction[i+1];
            if (rot > 0) {
                task_segList[i].lspeed = -ROTV;
                task_segList[i].rspeed = ROTV;
            } else {
                task_segList[i].lspeed = ROTV;
                task_segList[i].rspeed = -ROTV;
            }
            task_segList[i].rotangle = rot;
            task_segList[i].dur_time = compute_t(task_segList[i].rotangle,
                                                 DD/2,task_segList[i].lspeed);
            continue;
        }
        //直线
        if (t.trace->direction[i] == t.trace->direction[i+1]) {
            if (t.trace->inverse[i] == 1) { //反向
                if (vleft != vright) {
                    if (fabs(vleft) > fabs(vright)) {
                        if (vleft < 0)
                            vright = vleft;
                        else {
                            vleft = ADDV;
                            vright = ADDV;
                        }
                    } else {
                        if (vright < 0)
                            vleft = vright;
                        else {
                            vleft = ADDV;
                            vright = ADDV;
                        }
                    }
                }
                task_segList[i].lspeed = -ROTV;
                //vleft = task_segList[i].lspeed;
                task_segList[i].rspeed = -ROTV;
                //vright = task_segList[i].rspeed;
            }

            else { //正向
                if (vleft != vright) {
                    if (fabs(vleft) > fabs(vright)) {
                        if (vleft > 0)
                            vright = vleft;
                        else {
                            vleft = -ADDV;
                            vright = -ADDV;
                        }
                    } else {
                        if (vright > 0)
                            vleft = vright;
                        else {
                            vleft = -ADDV;
                            vright = -ADDV;
                        }
                    }
                }
                task_segList[i].lspeed = ROTV;
                //vleft = task_segList[i].lspeed;
                task_segList[i].rspeed = ROTV;
                //vright = task_segList[i].rspeed;
            }
            if (task_segList[i].lspeed > MAXV)
                task_segList[i].lspeed = MAXV;
            if (task_segList[i].lspeed < -MAXV)
                task_segList[i].lspeed = -MAXV;
            if (task_segList[i].rspeed > MAXV)
                task_segList[i].rspeed = MAXV;
            if (task_segList[i].rspeed < -MAXV)
                task_segList[i].rspeed = -MAXV;

            double d = sqrt((t.trace->ps[i].x - t.trace->ps[i+1].x)
                            *(t.trace->ps[i].x - t.trace->ps[i+1].x)
                            +(t.trace->ps[i].y - t.trace->ps[i+1].y)
                            *(t.trace->ps[i].y - t.trace->ps[i+1].y));
            task_segList[i].dur_time = fabs(d / task_segList[i].lspeed);
        }
    }
}

/*
 *slice current task
 */
static void sliceTask(task t)
{
    mylogfd (MOTOFD, "[moto]int sliceTask\n");
    if (t.type == TASK_STOP)
        task_segNum = 1;
    else if (t.type == TASK_ROTA||t.type == TASK_WITH_BALL) {
        if (t.rotslice == 0)
            task_segNum = 1;
        else
            task_segNum = (int)fabs(t.rotangle/t.rotslice) + 1;
    } else if (t.type == TASK_DOT_SHOOT) {
        task_segNum = t.trace->pnum;
    } else
        task_segNum = t.trace->pnum - 1;

    task_segList = (struct task_seg*)malloc(task_segNum * sizeof(struct task_seg));
    memset(task_segList, 0, task_segNum * sizeof(struct task_seg));

    if (task_segList == NULL) {
        mylogfd(MOTOFD,"[moto]sendTask: error in allocating memory\n");
        return;
    }

    task_segIdx = 0;

    mylogfd(MOTOFD,"[moto]sliceTask over %d\n",task_segNum);
    //setup segList
    if (t.type == TASK_STOP)  {
        stopTask_init(t);
        return;
    } else if (t.type == TASK_ROTA) {
        rotaTask_init(t);
        return;
    } else if (t.type == TASK_WITH_BALL) {
        withballTask_init(t);
        return;
    } else if (t.type == TASK_RUN) {
        runTask_init(t);
        return;
    } else if (t.type == TASK_CATCH_BALL) {
        catchballTask_init(t);
        return;
    } else if (t.type == TASK_DOT_SHOOT)
        dotshootTask_init(t);
    return;
}

static void motor_thread_main()
{
    motor_init();
    while (1) {

        pthread_mutex_lock(&task_mutex);
        if (mutex_flag == 1) {
            // sendSTOP();
            // mylogfd (-1, "%s %d\n", "----", task_segNum);
            if (task_segList != NULL) {
                free(task_segList);
                task_segList = NULL;
            }
            sliceTask(current);
            mylogfd (1, "%s %d\n", "New Task", task_segNum);
            mutex_flag = 0;
        }
        pthread_mutex_unlock(&task_mutex);
//mylogfd (1, "[moto] %d %d\n", task_segIdx, task_segNum);
        if (task_segIdx >= task_segNum)	{
            mylogfd(MOTOFD,"[moto]main:job done\n");
            pthread_mutex_lock(&task_mutex);
            job_done = 1;
	    job_over = 1;
            mylogfd (1, "[moto] job done\n");
            pthread_mutex_unlock(&task_mutex);
            //		sendSTOP();
            //sendAA(preleft,preright,0);
            pthread_mutex_lock(&stop_mutex);
        } else {
            //mylogfd(1,"x");
            if (task_segIdx == 0) {
                if (((fabs(preleft - task_segList[task_segIdx].lspeed) > MINV)
                        ||(fabs(preright - task_segList[task_segIdx].rspeed) > MINV))
                        &&(fabs(preleft > 10))) {
                    preleft = preleft/2;
                    preright = preright/2;
                    sendAA(preleft,preright,0);
                    usleep(1000000 * 0.1);
                    continue;
                }
            }
            //mylogfd(-1,"[Control]/motor/main:deal with the task_seg\n");
            if (task_segList[task_segIdx].type == TASK_ROTA) {
                if (task_segList[task_segIdx].dur_time == 0) {
                    mylogfd(1,"time = 0\n");
                    task_segIdx ++;
                    continue;
                }
                sendA(task_segList[task_segIdx].lspeed,
                      task_segList[task_segIdx].rspeed,0,1);
                usleep(1000000 * task_segList[task_segIdx].dur_time);
            } else if (task_segList[task_segIdx].type == TASK_STOP) {
                sendSTOP();
            } else if (task_segList[task_segIdx].type == TASK_WITH_BALL) {
                sendAA(task_segList[task_segIdx].lspeed,
                       task_segList[task_segIdx].rspeed,0);
                usleep(1000000 * task_segList[task_segIdx].dur_time);
            } else if (task_segList[task_segIdx].type == TASK_RUN) {
                sendAA(task_segList[task_segIdx].lspeed+left_flag,
                       task_segList[task_segIdx].rspeed+right_flag,0);
                usleep(1000000 * task_segList[task_segIdx].dur_time);
            } else if (task_segList[task_segIdx].type == TASK_CATCH_BALL) {
                sendAA(task_segList[task_segIdx].lspeed,
                       task_segList[task_segIdx].rspeed,0);
                usleep(1000000 * task_segList[task_segIdx].dur_time);
            }
            preleft = task_segList[task_segIdx].lspeed;
            preright = task_segList[task_segIdx].rspeed;
            task_segIdx++;
        }
    }
}

/*
 *send AA_DISTANCE to motor,left = vleft,right = vright,设置buf后通过msend发送
 */
void sendAA(int left, int right, int a)
{
    buf[0] = HEAD3;
    buf[1] = left;
    buf[2] = right;
    buf[3] = a;
    buf[4] = AA_DISTANCE;
    msend(buf, 5);
    mylogfd(-1,"<%d,%d>", left, right);
    vleft = left;
    vright = right;
}
/*
 *type == 0,AA;type == 1,AA_ANGLE;type == 2,AA_DISTANCE.
 */
void sendA(int left,int right,int a, int type)
{
    buf[0] = HEAD3;
    buf[1] = left/10;
    buf[2] = right;
    buf[3] = a;
    if (type == 1)
        buf[4] = AA_ANGLE;
    else if (type ==0)
        buf[4] = AA;
    else if (type == 2)
        buf[4] = AA_DISTANCE;
    msend(buf,5);
    //mylogfd(-1,"<%d,%d>%d", left, right, type);
    vleft = left;
    vright = right;
}

/*
 *main thread调用此函数添加task
 */
void sendTask(task t)
{
    mylogfd(MOTOFD,"[moto]sendTask\n");
    pthread_mutex_lock(&task_mutex);
    job_done = 0;
    job_over = 0;
    if (mutex_flag == 0)
        pthread_mutex_unlock(&stop_mutex);
    mutex_flag = 1;
    //if (current.trace != NULL) 		freeTrace (current.trace);
    current = t;
    pthread_mutex_unlock(&task_mutex);
    mylogfd(MOTOFD,"[moto]sendTask over\n");
}
void sendLeft()
{
    pthread_mutex_lock(&task_mutex);
    left_flag = LITV;
    right_flag = 0;
    pthread_mutex_unlock(&task_mutex);
}
void sendRight()
{
    pthread_mutex_lock(&task_mutex);
    right_flag = LITV;
    left_flag = 0;
    pthread_mutex_unlock(&task_mutex);
}
void sendStopAdjust()
{
    pthread_mutex_lock(&task_mutex);
    left_flag = 0;
    right_flag = 0;
    pthread_mutex_unlock(&task_mutex);
}

void sendSTOP()
{
    buf[0] = STOP;
    msend(buf,1);
    mylogfd(MOTOFD,"[moto]sendSTOP\n");
    vleft = 0;
    vright = 0;
    mylogfd(1,"[moto]stop\n");
}
void motor_init()
{
    mylogfd (MOTOFD, "[moto]init\n");
    mylogfd(1, "in motor\n");
    // init ttys
    if ((fd = ttys_init(0)) < 0) {
        exit(-1);
    }
    else
	printf("serial init ok\n");
	mylogfd(1, "in motor 2\n");
    // init ctrl
    ctrl_init(fd);
    tcgetattr(0, &tio);
    // clean up at exit
    if (atexit(cleanup)) {
        mylogfd(2,"[moto]init:atexit error\n");
        exit(-1);
    }
    task_segList = NULL;
}
void motor_exit()
{
    sendSTOP();
    tcsetattr(0, TCSANOW, &tio);
}
/*
 *motor entrance
 */
void motor_thread_create()
{
    pthread_mutex_init(&task_mutex, NULL);
    pthread_mutex_init(&stop_mutex, NULL);
    pthread_mutex_lock(&stop_mutex);
    pthread_t mpid;
    pthread_create(&mpid , NULL , (void *) motor_thread_main , NULL);

}
/*
 *check if the job is done
 */
int checkDone()
{
    pthread_mutex_lock(&task_mutex);
    int t = job_done;
    pthread_mutex_unlock(&task_mutex);
    return t;
}
