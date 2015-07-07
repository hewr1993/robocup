/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef SCHED_H_
#define SCHED_H_
#include "common.h"

typedef enum {
    SEARCH_BALL,
	SEARCHING_BALL,
	SEARCHING_ADJUST,
	FORWARDING_BALL,
    CATCHING_BALL,
	BALL_CATCHED,
	SHOOT,
	SHOOTING,
	MOVING,
	CHANGE_PLACE,
    FINDING_DIR,
    FACE_DOOR,
    CHANGING_PLACE,
    FIND_PLACE,
    OBSTACLE,
    PROTECT_DOOR,
	DONE,
	GAME,
	GAME_DOOR,
	OUTSIDE,
	SHOOTED,
	STATE_OVER,
	START                         // 状态机添加 -- redwalker
} ROBOT_STATE;

typedef enum  {
	CMD_STOP,
    CMD_POSITION,
	CMD_SEARCH_BALL,
    CMD_CATCH_BALL,
    CMD_ROTA_WITH_BALL,
	CMD_SHOOT,
    CMD_RUN,
    CMD_ROTA,
    CMD_FORWARD,
    CMD_OBSTACLE,
	CMD_NO
} COMMAND;

typedef struct video_parse_info{
	int is_valid;
	int num;
	int is_door_seen;
	Point door_pos;
	Point door_acc_pos;
	int is_door_right;
	Point door_right;
	int is_door_left;
	Point door_left;
	
	int is_edge_valid;
	Point edge_info;
	int green_sum;
	int is_ball_acc;//是否可以精确定位球
	Point ball_acc_pos;//球的精确位置
	
	int is_ball_seen;//是否可以看见球
	Point ball_pos;//大概位置
	int is_robot_seen;
	Point robot_pos;
	int is_robot_acc;
	Point robot_acc_pos;
	int is_outside;
}video_parse_info;

typedef struct SchedVar {
	int wait;//等待丢球
	int wait_for;//等待前进
	int waitstop;//等待找球停止
	int waitlost;
	int search_dir;
    int waitforward;
    int is_ball_seen;
    int is_door_seen;
    int doorseen;
    int door_dir;
    int search_door_dir;
    int count;//帧数计数器
    int door_dis;//看到门的最小距离
    int door_count;//看到门的帧数
    int max_green_sum;//绿色最多的数量
    int max_green_count;//绿色最广阔的帧数
}SchedVar;
enum{
    CONN_START = 1,
    NEW_MSG = 2
};
typedef struct Info {
    video_parse_info* video_info;//从video传过来的信息
    video_parse_info* video_info2;//从video传过来的信息
    SchedVar sched_var;

    ROBOT_STATE state;//机器人当前状态
      
    //上一次任务信息，目的地点和方向， 障碍物
    COMMAND last_cmd;  
    Point destP;
    int destdir;
    Point obstacle[3];//障碍物信息

    Point robotP;//机器人坐标
    int direction;//机器人方向,

    int speed[2];//机器人左右轮速度
    Point ballP;//球坐标
/*
    //从队友传过来的信息
    Point net_robotP;//机器人坐标
    Point net_ballP;//球坐标
    int net_direction;//机器人方向
    int net_speed[2];//机器人左右轮速度

    //敌方信息
    Point enemy[2];
*/
    //网络连接
    int connected;//是否与队友连接上
    int c_flag;
    char msg[16];
    struct net dest;
    char *ip;
    char *destip;
    
}Info ;

int POSITI;                            // 定义全局变量 -- redwalker
int START_FLAG;                        // 初始位置 -- redwalker
int job_over;                         // 执行完任务 --redwalker
// sched.c
void info_init(Info * info);
void sched (Info * info);

//task.c
int do_search_ball(Info * info, int dir);//找球 dir=1逆时针， 0顺时针
int do_position(Info * info, int dir);//机器人定位
int do_run(Info * info);//运动
int do_forward(Info *info, int maxa);//直线前进
int do_shoot(Info *info);//射门
int do_stop(Info *info);//停止
//angle旋转角度+-360， +为逆时针，-为顺时针;slice为是否分片，>0为分片大小，0不分片;
int do_rota(Info *info, int angle, int slice);
int do_rota_speed(Info *info, int angle, int slice, int speed);
int do_obstacle (Info *info);//避障
int do_catch_ball (Info *info);
int do_rota_with_ball (Info *info, int angle, int slice);//旋转角度angle， 分片大小slice
int do_adjust (int dir);//dir=1逆时针， 0顺时针,-1为停止
int do_dotshoot (Info *info, int pos, int ball); //点球
double get_time (int v, int distance);
int run_forward (int v, int distance);
double rota_time (int v,int angle );
int run_rota(int angle,int dir);
//int run_rotaR(int angle);


int do_sendmsg_to_teammate(Info *info, char *msg);//与队友通
#endif

