/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "sched.h"
#include "motor_control.h"
#define OBSTACLE_DIS (DD+DD/2)
static const int TASKFD = -2;
#define DEGREE_ARC (PI/180)
#define DOT_SHOOT_DIS 100
#define SHOOT_A 70 

/*
 * (x+yi) * (cosa+isina)
 */
static Point rotaPoint (Point p, int angle) {
    float arc = DEGREE_TO_ARC(angle);
    Point ret;
    ret.x = p.x*cos(arc) - p.y*sin(arc);
    ret.y = p.x*sin(arc) + p.y*cos(arc);
    return ret;
}
static Point getPointbyAnge (int angle) {
    int dis = 200;
    float arc = DEGREE_TO_ARC (angle);
    Point ret = {
        .x = dis * cos(arc),
        .y = dis * sin(arc),
    };
    return ret;
}
static void setPoint (Point *p, int x, int y) {
    p->x = x;
    p->y = y;
}
static void setDvidePoint (Point *p, Point *s, Point *e, int m, int sum) {
    
    p->x = ((sum-m) * s->x + m * e->x)/ sum;
    p->y = ((sum-m) * s->y + m * e->y)/ sum;
}
static int  getProduct(Point a, Point b) {
    return a.x*b.x + a.y*b.y;
}
static int  Isobstacle ( Point startP, int startD, Point endP, 
                        int endD, Point *obstacle, Point *limit){
    Point destP, obsP;
    setPoint (&destP, endP.x-startP.x, endP.y-startP.y );
    setPoint (&obsP, obstacle[0].x-startP.x, obstacle[0].y-startP.y );
    Point N;
    setPoint (&N, -destP.y, destP.x);
    int LN = getDistance(N);
    int product = getProduct (obsP, N);
    int dis = product/LN;
    if (dis < 0) {
        N.x = -N.x;
        N.y = -N.y;
        dis = -dis;
    }
    /* mylogfd (-1, "[%d]N=(%d,%d)obs=(%d,%d)\n", dis, N.x,N.y,obsP.x,obsP.y); */
    if (dis > OBSTACLE_DIS){
        mylogfd (-1, "no obstacle %d\n", dis);
        return 0;
    }
    limit->x = (2*obsP.x + startP.x)/3 + N.x * DD/LN;
    limit->y = (2*obsP.y + startP.y)/3 + N.y * DD/LN;
    /* mylogfd (-1, "startP(%d,%d)%d,endP(%d,%d)%d,limit(%d,%d)\n", startP.x,startP.y, */
    /*          startD, endP.x, endP.y, endD, limit->x, limit->y); */
    return 1;
}

static Trace *trace_plan_line(Point startP, int startD, Point endP, int endD){
    Point pp;
    setPoint (&pp, endP.x-startP.x, endP.y-startP.y );
    int distance = getDistance (pp);
    int angle = getAngle (pp);
    int num = 2+distance/SEGDIS;
    Trace *tr = newTrace (num);
 
    tr->ps[0] = startP;
    tr->ps[1] = startP;
    tr->ps[num-1] = endP;
    tr->direction[0] = startD;
    int i;
    for (i=1; i < num; ++i)
        tr->direction[i] = angle;
    for (i=2; i < num-1; ++i)
        setDvidePoint (&(tr->ps[i]), &startP, &endP, i-1, num-2);
    if (pp.x < 0) {
        for (i = 0; i < num; ++i)
            tr->inverse[i] = 1;
    }
    return tr;
};
static Trace *trace_plan_arc(Point startP, int startD, Point endP, int endD){
    

    mylogfd (TASKFD, "startP(%d,%d)%d,endP(%d,%d)%d\n", startP.x,startP.y,
             startD, endP.x, endP.y, endD);
    Point pp;
    int sign = 1;
    setPoint (&pp, endP.x-startP.x, endP.y-startP.y );
    if (pp.y < 0 ) sign = -1;
    int distance = getDistance (pp);
    //mylogfd(-1, "distance=%d ", distance);
    int angle = abs(getAngle (pp) - startD);
    if (angle == 0)
        return trace_plan_line (startP,startD,endP,endD);
    //mylogfd(-1, "angle=%d ", angle);
    int r = abs(distance/2/(sin(DEGREE_TO_ARC (angle))));
    //mylogfd(-1, "r=%d ", r);
    Point center = getPointbyAnge (startD + sign * 90);
    int dis = getDistance (center);
    //mylogfd(-1, "dis=%d ", dis);
    setPoint (&center, startP.x + r*center.x/dis, startP.y + r*center.y/dis);
    int angleseg = 3*180/(PI*r);
    //mylogfd(-1, "seg=%d ", angleseg);
    if (angleseg < 1) angleseg = 2;
    int num = 1 + angle*2/angleseg;
    //mylogfd(-1, "num=%d \n", num);
    Trace *tr = newTrace (num);
    mylogfd (TASKFD, "trace-arc angle=%d,center(%d,%d),angleseg=%d,num=%d,sign=%d,r=%d\n", angle, center.x,center.y, angleseg,num,sign,r);
    tr->ps[0] = startP;
    tr->ps[num-1] = endP;
    tr->direction[0] = startD;
    int i;
    for (i=1; i < num; ++i) {
        tr->direction[i] = tr->direction[i-1]+sign * angleseg;
        //mylogfd (-1, "%d,",tr->direction[i]);
    }
    for (i=0; i < num; ++i) {
        tr->r[i] = r;
    }
    setPoint (&pp, startP.x-center.x, startP.y-center.y );
    Point tmp;
    for (i=1; i < num-1; ++i) {
        tmp = rotaPoint (pp, sign *angleseg * i);
        setPoint (&(tr->ps[i]), center.x + tmp.x, center.y+tmp.y);
    }
    /* for (i=0; i < tr->pnum; ++i) */
    /*     mylogfd (-1, "%d(%d,%d)", i,tr->ps[i].x, tr->ps[i].y); */
    /* mylogfd (-1, "\n\n"); */
    return tr;
};
static Trace *trace_plan_obstacle(Point startP, int startD, Point endP, 
                                         int endD, Point *obstacle){
    Point limitP;
    int ret = Isobstacle (startP, startD, endP, endD, obstacle, &limitP);
    if ( ret == 0 )
        return trace_plan_line (startP, startD, endP, endD);
    Trace *tr1 = trace_plan_arc(startP, startD, limitP, endD);
    Trace *tr2 = trace_plan_arc(limitP, tr1->direction[tr1->pnum-1], endP, endD);
    Trace *tr = newTrace (tr1->pnum + tr2->pnum -1); 
    memcpy (tr->ps, tr1->ps, sizeof(Point) * tr1->pnum);
    memcpy (tr->direction, tr1->direction, sizeof(int) * tr1->pnum);
    memcpy (tr->inverse, tr1->inverse, sizeof(int) * tr1->pnum);
    memcpy (tr->r, tr1->r, sizeof(int) * tr1->pnum);
    int i = 0;
    int pnum = tr1->pnum;
    for (i=0; i < tr2->pnum-1; ++i) {
        tr->ps[pnum+i] = tr2->ps[i+1];
        tr->inverse[pnum+i] = tr2->inverse[i+1];
        tr->direction[pnum+i] = tr2->direction[i+1];
        tr->r[pnum+i] = tr2->r[i+1];
    }
    for (i=0; i < tr->pnum; ++i) {
        mylogfd (-1, "%d(%d,%d)",i, tr->ps[i].x, tr->ps[i].y);
    }
    mylogfd (-1, "\n");
    freeTrace (tr1);
    freeTrace (tr2);
    return tr;
};


int do_search_ball(Info * info, int dir){
    mylogfd (TASKFD, "[task]%s dir=%d\n", "do_search_ball", dir);
    if (dir == 0)
        do_rota(info, -360, 1);
    else if (dir > 0)
        do_rota(info, 360, dir);
    else
        do_rota(info, -360, -dir);
    mylogfd (TASKFD, "[task]%s dir=%d over\n", "do_search_ball", dir);
    info->last_cmd = CMD_SEARCH_BALL;
    return 0;
};
int do_position(Info * info, int dir){
    mylogfd (TASKFD, "[task]%s dir=%d\n", "do_position", dir);
    if (dir == 0)
        do_rota(info, -360, 1);
    else if (dir > 0)
        do_rota(info, 360, dir);
    else
        do_rota(info, -360, -dir);
    info->last_cmd = CMD_POSITION;
    return 0;
};
int do_run(Info * info){
    mylogfd (TASKFD, "[task]%s dest(%d %d) dir(%d,%d)\n", "do_run",info->destP.x,
             info->destP.y, info->direction, info->destdir);
    Task t;
    t.type = TASK_RUN;
    t.trace = trace_plan_arc (info->robotP, info->direction, 
                          info->destP, info->destdir);
    t.maxa = DEFAULT_A;
    sendTask(t);
    info->last_cmd = CMD_RUN;
    return 0;
};
int do_getin(Info * info,int dir) {
    
    mylogfd (TASKFD, "[task]%s\n", "do_getin");
    do_catch_ball(info);
    do_rota(info,180,dir);
    return 0;
};
int do_forward(Info * info, int maxa){
    Task t;
    t.type = TASK_RUN;
    Point directionP = {
        .x = info->destP.x - info->robotP.x,
        .y = info->destP.y - info->robotP.y,
    };
    info->destdir = getAngle (directionP);
    t.trace = trace_plan_line (info->robotP, info->direction, 
                                 info->destP, info->destdir);
    mylogfd (TASKFD, "[task]%s des(%d,%d) dir(%d %d)\n", "do_forward", 
             info->destP.x, info->destP.y, info->direction, info->destdir);
    t.maxa = maxa;
    sendTask(t);
    info->last_cmd = CMD_FORWARD;
    return 0;
};
int do_obstacle (Info *info){
    mylogfd (TASKFD, "[task]%s dest(%d %d) obstacle(%d,%d)\n", "do_obstacle",
             info->destP.x,info->destP.y,info->obstacle[0].x,info->obstacle[0].y);
    Task t;
    t.type = TASK_RUN;
    t.trace = trace_plan_obstacle (info->robotP, info->direction, 
                          info->destP, info->destdir, info->obstacle);
    t.maxa = DEFAULT_A;
    sendTask(t);
    info->last_cmd = CMD_OBSTACLE;
    return 0;
}
int do_catch_ball (Info *info) {
    mylogfd (TASKFD, "[task]%s (%d,%d)\n", 
             "do_catch_ball", info->destP.x, info->destP.y);
    Task t;
    t.type = TASK_CATCH_BALL;
    t.trace = trace_plan_line (info->robotP, info->direction, 
                               info->destP, info->destdir);
    t.maxa = DEFAULT_A;
    sendTask(t);
    info->last_cmd = CMD_CATCH_BALL;
    return 0;
}
int do_stop(Info *info){
    mylogfd (TASKFD, "[task]%s\n", "do_stop");
    Task t;
    t.type = TASK_STOP;
    sendTask(t);
    info->last_cmd = CMD_STOP;
    return 0;
};


int do_rota(Info *info, int angle, int slice){
    return do_rota_speed (info, angle, slice, 10);
};
int do_rota_speed(Info *info, int angle, int slice, int speed){
    mylogfd (TASKFD, "[task]%s angle=%d,slice=%d,speed=%d \n", 
             "do_rota", angle, slice, speed);
    Task t;
    t.type = TASK_ROTA;
    t.rotslice = slice;
    t.rotangle = angle;
    t.rota_speed = speed;
    sendTask(t);
    mylogfd (TASKFD, "[task]%s\n", "do_rota_speed over");
    info->last_cmd = CMD_ROTA;
    return 0;
};
int do_rota_with_ball (Info *info, int angle, int slice) {
    mylogfd (TASKFD, "[task]%s angle=%d,slice=%d \n", 
             "do_rota_with_ball", angle, slice);
    Task t;
    t.type = TASK_WITH_BALL;
    t.rotslice = slice;
    t.rotangle = angle;
    sendTask(t);
    info->last_cmd = CMD_ROTA_WITH_BALL;
    return 0;
};
//dir=1逆时针， 0顺时针 -1停止
int do_adjust (int dir) {
    static int last = -1;
    if (last == dir)
        return 0;
    else
        last = dir;
    //mylogfd (TASKFD, "[task]%s dir=%d\n", "do_adjust", dir);
    if (dir==1) sendRight();
	else if (dir == 0)sendLeft();
    else  sendStopAdjust ();
    return 0;
}

int do_shoot(Info *info){
    mylogfd (TASKFD, "[task]%s\n", "do_shoot");
    do_run (info);
    info->last_cmd = CMD_SHOOT;
    return 0;
};
int do_dotshoot_line (Info *info,int pos, int ball){
    const int SHOT_DIS = 25; 
    const int LINE_SHOT_DIS = 60; 
    int dis = LINE_SHOT_DIS + SHOT_DIS;
    int num = 2+ dis/SEGDIS;
    Trace *tr = newTrace (num);
    setPoint (&(tr->ps[0]), 0, 0);
    setPoint (&(tr->ps[1]), -LINE_SHOT_DIS, 0);
    setPoint (&(tr->ps[num-1]), SHOT_DIS, 0 );
    int i = 0;
    for (i = 0; i < num ; ++i) 
        tr->direction[i] = 0;
    tr->inverse[0] = 1;

    for (i = 2; i < num-1 ; ++i) {
        setDvidePoint (&(tr->ps[i]), &(tr->ps[1]), &(tr->ps[num-1]), i-1, num-2);
        mylogfd (-1, "%d(%d,%d)",i, tr->ps[i].x, tr->ps[i].y);
    }
    /* for (i=0; i < tr->pnum; ++i)  */
    /*     mylogfd (-1, "%d(%d,%d)",i, tr->ps[i].x, tr->ps[i].y); */
    
    Task t;
    t.type = TASK_DOT_SHOOT;
    t.maxa = SHOOT_A;
    t.trace = tr;
    mylogfd (-1, "pnum=%d\n",t.trace->pnum);
    sendTask(t);
    return 0;
};
int do_dotshoot (Info *info, int pos, int ball) {
    if (pos < 10 && pos > -10)
        return do_dotshoot_line (info, pos, ball);
    int sign = 1;
    if (pos < 0) sign = -1;
    float tana = abs(pos)*1.0f/ball;
    
    float a = atan(tana);
    float sina = sin(a);
    float cosa = cos(a); 
    float r = DOT_SHOOT_DIS/(1+1/sina);
    int angle = -sign * ARC_TO_DEGREE(a);
    Point p = {
        .x = -DOT_SHOOT_DIS+r+r*sina,
        .y = r*cosa * (-sign),
    };
    //mylogfd (-1, "tana=%f,p=(%d,%d)",tana,p.x,p.y);
    int dis = r/tana;
    int num = 4 + dis/SEGDIS;
    //mylogfd (-1, "pos=%d,ball=%d,r=%d,angle=%d,num=%d,dis=%d\n",pos,ball,(int)r,angle,num,dis);
    Trace *tr = newTrace (num);
    setPoint (&(tr->ps[0]), 0, 0);
    setPoint (&(tr->ps[1]), -DOT_SHOOT_DIS, 0);
    setPoint (&(tr->ps[2]), -DOT_SHOOT_DIS, 0);
    setPoint (&(tr->ps[3]), p.x, p.y);
    setPoint (&(tr->ps[num-1]), 0, 0 );
    tr->direction[0] = 0;
    tr->direction[1] = 0;
    tr->r[2] = r;
    tr->r[3] = r;
    tr->direction[2] = sign*90;
    tr->direction[3] = angle;
    tr->inverse[0] = 1;

    int i = 0;
    for (i = 4; i < num ; ++i) {
        tr->direction[i] = angle;
        setDvidePoint (&(tr->ps[i]), &p, &(tr->ps[0]), i-3, num-4);
        // mylogfd (-1, "%d(%d,%d)",i, tr->ps[i].x, tr->ps[i].y);
    }
    /* mylogfd (-1, "\n\n"); */
    /* for (i=0; i < tr->pnum; ++i)  */
    /*     mylogfd (-1, "%d(%d,%d)",i, tr->ps[i].x, tr->ps[i].y); */
    
    Task t;
    t.type = TASK_DOT_SHOOT;
    t.maxa = SHOOT_A;
    t.trace = tr;
    //mylogfd (-1,"\nzz %d\n", t.trace->pnum);
    sendTask(t);
    return 0;
};
int do_sendmsg_to_teammate(Info *info, char *msg) {
    mylogfd (TASKFD, "[task]%s %s\n", "do_sendmsg_to_teammate ", msg);
    net_send(msg, strlen(msg), &(info->dest));
    return 0;
};
double get_time (int v, int distance) {
    double t = distance * 1000000.0f/v;
    return t;
};
int run_forward (int v, int distance) {
    mylogfd (1, "run forward %d\t", distance);
	sendAA(v,v,0);
    double t = get_time (v, distance);
    mylogfd (-1, "usleep t=%f\t", t);
    usleep(t);
    mylogfd (-1, "stop\n");
    sendAA(0,0,0);
    return 0;
};
double rota_time (int v,int angle )
{
    //float a = (angle + 22.5) * 144.0f/141;
    float a = angle;
    //mylogfd (-1, "a=%f, angle=%d v=%d\n", a,angle, v);
    double t = a/360*DD*PI/v * ROTA_TIME;
    return t * 1000000;
};
int run_rota(int angle , int dir)
{
    mylogfd (1, " left rota\t");
    double t = rota_time (10, angle);
    sendA(-10 * dir,10 * dir,10,1);
    mylogfd (-1, "usleep t=%f\t", t);
    usleep(t);
    mylogfd (-1, "stop\n", t);
    sendAA(0,0,0);
    return 0;
};
/*int run_rotaR(int angle)
{
    mylogfd (1, " left rota\t");
    double t = rota_time (10, angle);
    sendA(10,-10,10,1);
    mylogfd (-1, "usleep t=%f\t", t);
    usleep(t);
    mylogfd (-1, "stop\n", t);
    sendAA(0,0,0);
    return 0;
};*/
//bool is_start = false;

