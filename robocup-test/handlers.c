/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "sched.h"
#include "motor_control.h"
#include <stdlib.h>
static const int CMDFD = -5;

extern int flag;
extern Info info;
extern video_parse_info video_tmp[2];
extern int kbctrl(int argc, char **argv);
static void output_video_info(video_parse_info video_tmp);


static const char usage[] = \
    "--------------------------------------------\n"\
    "\tcall: ignore all the input message fron net, video and stop schedular\n"\
    "\toall: open all\n"\
    "\tcvid: ignore the input from  video\n"\
    "\tovid: open the input from  video\n"\
    "\tcnet: ignore the input from net\n"\
    "\tonet: open the input from net\n"\
    "\tcsch: stop schedular\n"\
    "\tosch: restart schedular\n"\
    "\ttask: send task to robot\n"\
    "\tsbal: search ball\n"\
    "\tposn: the robot position itself\n"\
    "\tshot: shoot at the goal \n"\
    "\tstop: stop robot\n"\
    "\trota: robot rotation\n"\
    "\trun : robot run\n"\
    "\tvido: decode the information from videocap\n"\
    "\tmoto: control the motor all by keyboard\n"\
    "\tnet : decode the information from net\n"\
    "\thelp: show this message\n";
int cmd_help         (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_help:%s\n", usage);
    return 0;
};
int cmd_quit         (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_quit\n");
    raise (SIGINT);
    //exit(1);
    return 0;
};
int cmd_close_all    (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_close all\n");
    flag &= FLAG_NONE;
    return 0;
};
int cmd_close_video  (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_close video\n");
    flag &= ~FLAG_VIDEO;
    return 0;
};
int cmd_close_net    (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_close_net\n");
    flag &= ~FLAG_NET;
    return 0;
};
int cmd_close_sched  (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_close_schedular\n");
    flag &= ~FLAG_SCHED;
    return 0;
};
int cmd_open_video   (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_open_video\n");
    flag |= FLAG_VIDEO;
    return 0;
};
int cmd_open_net     (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_open_net\n");
    flag |= FLAG_NET;
    return 0;
};
int cmd_open_sched   (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_open_schedular\n");
    flag |= FLAG_SCHED;
    return 0;
};
int cmd_open_all     (int argc, char **argv){
    mylogfd(CMDFD,"[cmd]stdin : %s \n",argv[0]);
    mylogfd (CMDFD, "[cmd]cmd_open_all\n");
    POSITI = atoi(argv[0]);               //参数传入 --redwalker
    //printf("POSITI = %d",POSITI);
    mylogfd (CMDFD, "[cmd]POSITI = %d\n",POSITI);
    START_FLAG = 0;
    flag |= FLAG_ALL;
	
    return 0;
};
int cmd_task         (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_task\n");
    return 0;
};
int cmd_search_ball  (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_search_ball\n");
    do_search_ball(&info, 1);
    return 0;
};
int cmd_position     (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_position the robot itself\n");
    do_position(&info, 1);
    return 0;
};
int cmd_shoot        (int argc, char **argv){
    if (argc < 2) {
        mylogfd (CMDFD, "[cmd]cmd_dotashoot pos,ball\n");
        return 0;
    }
    info.robotP.x = 0;
    info.robotP.y = 0;
    info.destP.x = 0;
    info.destP.y = 0;
    int pos = atoi(argv[0]);
    int ball = atoi(argv[1]);
    mylogfd (CMDFD, "[cmd]cmd_dotashoot\n");
    do_dotshoot(&info,pos,ball);
    return 0;
};
int cmd_stop         (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_stop\n");
    do_stop(&info);
    return 0;
};
int cmd_init         (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_init %d\n", argc);
    if (argc >= 1)
        motor_init();
    info_init (&info);
    return 0;
};
/*
 * args: angle [slice]
 * angle:旋转角度+-360， +为逆时针，-为顺时针, 默认为45
 * slice: 为是否分片，>0为分片大小，0不分片;默认为0 
 */
int cmd_rota         (int argc, char **argv){
    int angle = 45;
    int slice = 0;
    if (argc > 0)
        angle = atoi (argv[0]);
    if (argc > 1)
        angle = atoi (argv[1]);
    mylogfd (CMDFD, "[cmd]cmd_rotation angle=%d, slice=%d\n");
    do_rota (&info, angle, slice);
    return 0;
};
int cmd_goto         (int argc, char **argv){
    if (argc < 2) {
        mylogfd (CMDFD, "[cmd]cmd_goto x, y\n");
        return 0;
    }
    info.robotP.x = 0;
    info.robotP.y = 0;
    info.destP.x = atoi(argv[0]);
    info.destP.y = atoi(argv[1]);
    if (argc >=3) 
        info.destdir = atoi(argv[2]);
    else
        info.destdir = getAngle (info.destP);

    mylogfd (CMDFD, "[cmd]cmd_goto (%d, %d) \n", info.destP.x, info.destP.y);
    do_forward (&info, DEFAULT_A);
    return 0;
};
int cmd_run          (int argc, char **argv){
    if (argc < 2) {
        mylogfd (CMDFD, "[cmd]cmd_run x, y\n");
        return 0;
    }
    info.robotP.x = 0;
    info.robotP.y = 0;
    info.destP.x = atoi(argv[0]);
    info.destP.y = atoi(argv[1]);
    if (argc < 4) {
        mylogfd (CMDFD, "[cmd]cmd_run (%d, %d)\n", info.destP.x, info.destP.y);
        do_run (&info);
        return 0;
    }
    info.obstacle[0].x = atoi(argv[2]);
    info.obstacle[0].y = atoi(argv[3]);
    mylogfd (CMDFD, "[cmd]cmd_run to(%d, %d) obstacle(%d %d)\n", 
             info.destP.x, info.destP.y,
             info.obstacle[0].x, info.obstacle[0].y);
    do_obstacle (&info);
    return 0;
};
int cmd_sendAA       (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_sendAA \n");
    int vleft,vright;
    int a = 0;
    int type = AA;
    vleft = atoi(argv[0]);
    vright = atoi (argv[1]);

    if (argc > 2)
        a = atoi (argv[2]);
    if (argc > 3) {
        if (argv[3][0] == '0')
            type = AA;
        else 
            type = AA_ANGLE;
    }
    sendA (vleft, vright, a, type);
    return 0;
};
int cmd_motor        (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_moto for motor test \n");
    int flag_save = flag;
    cmd_close_all (argc, argv);
    kbctrl(argc, argv); 
    flag = flag_save;
    return 0;
};
int cmd_video        (int argc, char **argv){
    int i=0;
    int num=0;
    /*int  j = 0;
    //for (j=0; j < argc; ++j) 
    //  mylogfd (1, "<%s>", argv[j]);
    
    //mylogfd (1,"\n");
    if(argc <= 1)
    video_tmp.is_valid=0;
	else {
    video_tmp.is_valid = 1;
    video_tmp.num = atoi(argv[0]);
	}*/
	while(i<argc){
		switch(argv[i][0]){
        case 'n':
            if(atoi(argv[++i])<2)
                num=atoi(argv[i]);
            // mylogfd (-1, "[Handler] b: %d, %d , %d. %d \n",video_tmp.is_ball_seen,video_tmp.ball_pos.x,video_tmp.ball_pos.y,i);
            break;
        case 'b':
            if(atoi(argv[++i])==-10000)
                video_tmp[num].is_ball_seen=0;
            else{
                video_tmp[num].is_ball_seen=1;
                video_tmp[num].ball_pos.x=atoi(argv[i]);
                video_tmp[num].ball_pos.y=atoi(argv[++i]);
            }
            // mylogfd (-1, "[Handler] b: %d, %d , %d. %d \n",video_tmp.is_ball_seen,video_tmp.ball_pos.x,video_tmp.ball_pos.y,i);
            break;
        case 'd':
            if(atoi(argv[++i])==-10000)
                video_tmp[num].is_door_seen=0;
            else{
                video_tmp[num].is_door_seen=1;
                video_tmp[num].door_pos.x=atoi(argv[i]);
                video_tmp[num].door_pos.y=atoi(argv[++i]);
            }
            break;
            //mylogfd (-1, "[Handler] d: %d, %d , %d. %d \n",video_tmp.is_door_seen,video_tmp.door_pos.x,video_tmp.door_pos.y,i);
        case 'l':
            if(atoi(argv[++i])==-10000)
                video_tmp[num].is_door_left=0;
            else{
                video_tmp[num].is_door_left=1;
                video_tmp[num].door_left.x=atoi(argv[i]);
                video_tmp[num].door_left.y=atoi(argv[++i]);
            }
            break;
            //mylogfd (-1, "[Handler] l: %d, %d , %d, %d \n",video_tmp.is_door_left,video_tmp.door_left.x,video_tmp.door_left.y,i);
        case 'r':
            if(atoi(argv[++i])==-10000)
                video_tmp[num].is_door_right=0;
            else{
                video_tmp[num].is_door_right=1;
                video_tmp[num].door_right.x=atoi(argv[i]);
                video_tmp[num].door_right.y=atoi(argv[++i]);
            }
            break;
            //mylogfd (-1, "[Handler] r: %d, %d , %d, %d \n",video_tmp.is_door_right,video_tmp.door_right.x,video_tmp.door_right.y,i);
         case 'e':
            if(atoi(argv[++i])==-10000)
                video_tmp[num].is_edge_valid=0;
            else{
                video_tmp[num].is_edge_valid=1;
                video_tmp[num].edge_info.x=atoi(argv[i]);
                video_tmp[num].edge_info.y=atoi(argv[++i]);
            }
           // if(video_tmp[num].is_edge_valid)
           //     printf("[handler]x = %d, y = %d\n",  video_tmp[num].edge_info.x, video_tmp[num].edge_info.y);
            break;
        case 'p':
            if(atoi(argv[++i])==-10000)
                video_tmp[num].is_ball_acc=0;
            else{
                video_tmp[num].is_ball_acc=1;
                video_tmp[num].ball_acc_pos.x=atoi(argv[i]);
                video_tmp[num].ball_acc_pos.y=atoi(argv[++i]);
            }
            break;
         case 'a':
            if(atoi(argv[++i])==-10000)
                video_tmp[num].is_robot_acc=0;
            else{
                video_tmp[num].is_robot_acc=1;
                video_tmp[num].robot_acc_pos.x=atoi(argv[i]);
                video_tmp[num].robot_acc_pos.y=atoi(argv[++i]);
            }
            break;
         case 'o':
            if(atoi(argv[++i])==-10000)
                video_tmp[num].is_robot_seen=0;
            else{
                video_tmp[num].is_robot_seen=1;
                video_tmp[num].robot_pos.x=atoi(argv[i]);
                video_tmp[num].robot_pos.y=atoi(argv[++i]);
            }
            break;
         case 'c':
				video_tmp[num].is_outside=atoi(argv[++i]);
			break;
		 case 'g':
				video_tmp[num].green_sum=atoi(argv[++i]);
			break;
		 case 'f':
				video_tmp[num].door_acc_pos.x=atoi(argv[++i]);
				video_tmp[num].door_acc_pos.y=atoi(argv[++i]);
			break;
        default:
            break;
		}	
		while ( i < argc) {
			if(argv[i++][0]=='#')
				break;
		}
	}
	info.video_info=&(video_tmp[0]);
	info.video_info2=&(video_tmp[1]);	
	//output_video_info(video_tmp[0]);
	//output_video_info(video_tmp[1]);
    //mylogfd (1, "the input from video \n");
    return 0;
};
int cmd_net          (int argc, char **argv){
    mylogfd (CMDFD, "cmd_net [%s]\n", argv[0]);
    int len = strlen (argv[0]);
    strncpy (info.msg, argv[0], len);
    info.msg[len] = '\0';
    return 0;
};
int cmd_sendmsg      (int argc, char **argv){
    if (argc < 1)
        return 0;
    mylogfd (CMDFD, "cmd_sendmsg %s\n", argv[0]);
    char buf[16];
    int len  = strlen (argv[0]);
    strncpy (buf, argv[0],len);
    buf[len] = '\0';
    //buf[len+1] = '\0';
    do_sendmsg_to_teammate (&info, buf);
    return 0;
};
static const char* state_usage[] = {
    "SEARCH_BALL",
    "SEARCHING_BALL",
    "SEARCHING_ADJUST",
    "FORWARDING_BALL",
    "CATCHING_BALL",
    "BALL_CATCHED",
    "SHOOT",
    "SHOOTING",
    "MOVING",
    "CHANGE_PLACE",
    "FINDING_DIR",
	 "FACE_DOOR",
    "CHANGING_PLACE",
    "FIND_PLACE",
    "OBSTACLE",
    "PROTECT_DOOR",
    "DONE",
    "GAME",
    "GAME_DOOR",
    "SHOOTED",
    "OUTSIDE,",
    "STATE_OVER",
};
int cmd_state        (int argc, char **argv){
    int i = 0;
    if (argc < 1) {
        for (i = 0; i < STATE_OVER; ++i)
            mylogfd (1, "%d    %s\n", i, state_usage[i]);
        return 0;
    }
    mylogfd (CMDFD, "[cmd]cmd_state %s\n", argv[0]);
    do_stop(&info);
    info.state = atoi(argv[0]);
    return 0;
};
int cmd_test         (int argc, char **argv){
    char buf[16];
    int size  =10;
    if (argc >1) {
        size = atoi (argv[0]);
    }
    mylogfd (CMDFD, "cmd_test %d\n", size);
    net_recv(buf, size, &(info.dest));
    mylogfd (CMDFD, "recv [%s]\n", buf);
    return 0;
};
#define GROUND_X 400
#define GROUND_Y 450
int cmd_game         (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_game game begain\n");
    info.state = GAME;
    info.destP.x = 0;
    info.destP.y = 0;
    if (argc >= 2) {
        info.destP.x = atoi(argv[0]);
        info.destP.y = atoi(argv[1]);
    }
    do_forward (&info, MAX_A);
    cmd_open_all (argc, argv);
    return 0;
};
int cmd_game_door    (int argc, char **argv){
    mylogfd (CMDFD, "[cmd]cmd_game_door game begain\n");
    info.state = GAME_DOOR;
    info.destP.x =  5;
    info.destP.y = GROUND_Y/2;
    if (argc >= 2) {
        info.destP.x = atoi(argv[0]);
        info.destP.y = atoi(argv[1]);
    }
    do_run (&info);
    cmd_open_all (argc, argv);
    return 0;
};
static void output_video_info(video_parse_info video_tmp){
	if(video_tmp.is_ball_seen)
		mylogfd (-1, "[Handler] See the ball: %d , %d \n",video_tmp.ball_pos.x,video_tmp.ball_pos.y);
	else
		mylogfd (-1, "[Handler] Miss the ball\n");
	if(video_tmp.is_ball_acc)
		mylogfd (-1, "[Handler] The ball acc : %d , %d \n",video_tmp.ball_acc_pos.x,video_tmp.ball_acc_pos.y);
	else
		mylogfd (-1, "[Handler] Can't get the acc pos'\n");
	if(video_tmp.is_door_seen)
		mylogfd (-1, "[Handler] See the door: %d , %d \n",video_tmp.door_pos.x,video_tmp.door_pos.y);
	else
		mylogfd (-1, "[Handler] Miss the door\n");
	if(video_tmp.is_door_left)
		mylogfd (-1, "[Handler] See the door left: %d , %d \n",video_tmp.door_left.x,video_tmp.door_left.y);
	else
		mylogfd (-1, "[Handler] Miss the door left\n");
	if(video_tmp.is_door_right)
		mylogfd (-1, "[Handler] See the door right: %d , %d \n",video_tmp.door_right.x,video_tmp.door_right.y);
	else
		mylogfd (-1, "[Handler] Miss the door right\n");
}
