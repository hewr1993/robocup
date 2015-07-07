/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "motor_control.h"
#include "sched.h"
#include <stdlib.h>
#include <time.h>
#define SCHFD -4

#define SCHED_WAIT_TIME 3
#define SCHED_waitforward_TIME 2
#define DOOR_CENTER 20
const Point UNKNOW = {-1000, -1000};

int is_start = 0;

static int Random(int r) {
    return rand() % r;
}
static int Distance(int x1, int y1, int x2, int y2) {
		return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}
static int IsBallSeen(Info* info) {
    return info->sched_var.is_ball_seen;
   /* static int wait = 0;
    static int ret = 0;
	if (info->video_info->is_ball_seen || info->video_info2->is_ball_seen) {
		wait ++;
		if (wait >= 3 || ret == 1) {
			ret = 1;
			wait = 3;
		}
		return ret;
	} else {
		wait --;
		if (wait <= 0 || ret == 0) {
			ret = 0;
			wait = 0;
		}
		return ret;
	}
	return (info->video_info->is_ball_seen || info->video_info2->is_ball_seen);*/
}
static int IsLeft(Info * info) {
	if (!IsBallSeen(info)) return 0;
	return (info->video_info->ball_pos.x == LOC_LEFT || info->video_info2->ball_pos.x == LOC_LEFT);
}
static int IsRight(Info * info) {
	if (!IsBallSeen(info)) return 0;
	return (info->video_info->ball_pos.x == LOC_RIGHT || info->video_info2->ball_pos.x == LOC_RIGHT);
}
static int IsMiddle(Info * info) {
	if (!IsBallSeen(info)) return 0;
	return (info->video_info->ball_pos.x == 0 || info->video_info2->ball_pos.x == 0);
}
static int IsBallAllSeen(Info * info) {
    return (info->video_info->is_ball_seen && info->video_info2->is_ball_seen);
}
static int IsBallFar(Info * info) {
    return ((info->video_info->is_ball_seen 
             && info->video_info->ball_pos.y == LOC_Y_FAR)
            || (info->video_info2->is_ball_seen 
                && info->video_info2->ball_pos.y == LOC_Y_FAR));
}
static int IsBallNear(Info * info) {
    return ((info->video_info->is_ball_seen && info->video_info->ball_pos.y == LOC_Y_NEAR) ||
            (info->video_info2->is_ball_seen && info->video_info2->ball_pos.y == LOC_Y_NEAR));
}    
static int IsBallVeryNear(Info * info) {
    return ((info->video_info->is_ball_seen 
             && info->video_info->ball_pos.y == LOC_Y_VERY_NEAR)
            || (info->video_info2->is_ball_seen 
                && info->video_info2->ball_pos.y == LOC_Y_VERY_NEAR));
}    
static int x_bound(int x) {
       return (x >= -500 && x <= 500);
}
static int y_bound(int y) {
       return (y >= 0 && y <= 500);
}
static int getx(Info * info) {
    int ret = 0;
    int x1 = info->video_info->ball_acc_pos.x;
    int x2 = info->video_info2->ball_acc_pos.x;
    if (x_bound(x1) && x_bound(x2) && info->video_info->is_ball_seen && info->video_info2->is_ball_seen) ret = (x1 + x2) / 2;
    else if (x_bound(x1) && info->video_info->is_ball_seen) ret = x1;
    else if (x_bound(x2) && info->video_info2->is_ball_seen) ret = x2;

    return ret;
}

static int gety(Info * info) {
    int ret = 0;
    int y1 = info->video_info->ball_acc_pos.y;
    int y2 = info->video_info2->ball_acc_pos.y;
    if (y_bound(y1) && y_bound(y2) && info->video_info->is_ball_seen && info->video_info2->is_ball_seen && info->video_info->is_ball_acc && info->video_info2->is_ball_acc) ret = (y1 + y2) / 2;
    else if (y_bound(y1) && info->video_info->is_ball_seen && info->video_info->is_ball_acc) ret = y1;
    else if (y_bound(y2) && info->video_info2->is_ball_seen && info->video_info2->is_ball_acc) ret = y2;
    return ret;
}
static int IsBallCatch(Info * info) {   
    return ((info->video_info->is_ball_seen && info->video_info->ball_pos.y == LOC_CATCH_BALL) ||
            (info->video_info2->is_ball_seen && info->video_info2->ball_pos.y == LOC_CATCH_BALL));
} 
static int getDoorMidx(Info * info) {
    int ret = 0;
    int x1 = info->video_info->door_acc_pos.x;
    int x2 = info->video_info2->door_acc_pos.x;
    if (x_bound(x1) && x_bound(x2) && info->video_info->is_door_seen && info->video_info2->is_door_seen) ret = (x1 + x2) / 2;
    else if (x_bound(x1) && info->video_info->is_door_seen) ret = x1;
    else if (x_bound(x2) && info->video_info2->is_door_seen) ret = x2;

    return ret;
}
static int getDoorMidy(Info * info) {
	int ret = 0;
    int y1 = info->video_info->door_acc_pos.y;
    int y2 = info->video_info2->door_acc_pos.y;
    if (y_bound(y1) && y_bound(y2) && info->video_info->is_door_seen && info->video_info2->is_door_seen) ret = (y1 + y2) / 2;
    else if (y_bound(y1) && info->video_info->is_door_seen) ret = y1;
    else if (y_bound(y2) && info->video_info2->is_door_seen) ret = y2;
    
    return ret;
}   
static int getDoorLeftx(Info * info) {
    int ret = 0;
    int x1 = info->video_info->door_left.x;
    int x2 = info->video_info2->door_left.x;
    if (x_bound(x1) && x_bound(x2) && info->video_info->is_door_left && info->video_info2->is_door_left) ret = (x1 + x2) / 2;
    else if (x_bound(x1) && info->video_info->is_door_left) ret = x1;
    else if (x_bound(x2) && info->video_info2->is_door_left) ret = x2;
    return ret;
}   
static int getDoorLefty(Info * info) {
    int ret = 0;
    int y1 = info->video_info->door_left.y;
    int y2 = info->video_info2->door_left.y;
    if (y_bound(y1) && y_bound(y2) && info->video_info->is_door_left && info->video_info2->is_door_left) ret = (y1 + y2) / 2;
    else if (y_bound(y1) && info->video_info->is_door_left) ret = y1;
    else if (y_bound(y2) && info->video_info2->is_door_left) ret = y2;
    return ret;
}   
static int getDoorRightx(Info * info) {
    int ret = 0;
    int x1 = info->video_info->door_right.x;
    int x2 = info->video_info2->door_right.x;
    if (x_bound(x1) && x_bound(x2) && info->video_info->is_door_right && info->video_info2->is_door_right) ret = (x1 + x2) / 2;
    else if (x_bound(x1) && info->video_info->is_door_right) ret = x1;
    else if (x_bound(x2) && info->video_info2->is_door_right) ret = x2;
    return ret;
}   
static int getDoorRighty(Info * info) {
    int ret = 0;
    int y1 = info->video_info->door_right.y;
    int y2 = info->video_info2->door_right.y;
    if (y_bound(y1) && y_bound(y2) && info->video_info->is_door_right && info->video_info2->is_door_right) ret = (y1 + y2) / 2;
    else if (y_bound(y1) && info->video_info->is_door_right) ret = y1;
    else if (y_bound(y2) && info->video_info2->is_door_right) ret = y2;
    return ret;
}   
static int IsDoorSeen(Info * info) {
    return info->sched_var.is_door_seen;
 /*   static int wait = 0;
    static int ret = 0;
	if (info->video_info->is_door_seen || info->video_info2->is_door_seen) {
		wait ++;
		if (wait >= 3 || ret == 1) {
			ret = 1;
			wait = 3;
		}
		return ret;
	} else {
		wait --;
		if (wait <= 0 || ret == 0) {
			ret = 0;
			wait = 0;
		}
		return ret;
	}*/
}
static int IsDoorAllSeen(Info * info){
// mylogfd(SCHFD, "left eye is_door_seen = %d, right eye is_door_seen%d\n", info->video_info->is_door_seen, info->video_info2->is_door_seen);
    return (info->video_info->is_door_seen && info->video_info2->is_door_seen);
} 
static int IsDoor(Info * info) { 
    int dx = getDoorLeftx(info) - getDoorRightx(info);
    int dy = getDoorLefty(info) - getDoorRighty(info);
    mylogfd(SCHFD, "[sche]dx = %d, dy = %d\n", dx, dy);
    if (dx * dy > 2500) return 1;
    else return 0;
}
static int IsDoorLeft(Info * info) {
    return (IsDoorSeen(info) && (info->video_info->door_pos.x == LOC_LEFT || info->video_info2->door_pos.x == LOC_LEFT));
}
static int IsDoorRight(Info * info) {
    return (IsDoorSeen(info) && (info->video_info->door_pos.x == LOC_RIGHT || info->video_info2->door_pos.x == LOC_RIGHT));
}
static int IsDoorMiddle(Info * info) {
    return (abs(getDoorMidx(info)) < 50);
}
static int IsDoorArrived(Info * info) {
 //    mylogfd(SCHFD, "IsDoorArrived: %d %d %d %d %d %d\n", 
   //         info->video_info->is_door_seen, info->video_info->is_door_left, info->video_info->is_door_right, 
     //       info->video_info2->is_door_seen, info->video_info2->is_door_left, info->video_info2->is_door_right);
    // return (info->video_info->is_door_seen && !info->video_info->is_door_left && !info->video_info->is_door_right || info->video_info2->is_door_seen && !info->video_info2->is_door_left && !info->video_info2->is_door_right);
      return (getDoorMidy(info) < 10);
}
static int IsEdgeValid(Info * info) {
    return (info->video_info->is_edge_valid || info->video_info2->is_edge_valid);
}
static int getEdgeX(Info * info) {
    int ret = 0;
    int x1 = info->video_info->edge_info.x;
    int x2 = info->video_info2->edge_info.x;
    if (y_bound(x1) && y_bound(x2) && info->video_info->is_edge_valid && info->video_info2->is_edge_valid) 
        return min(x1, x2);
    else if (y_bound(x1) && info->video_info->is_edge_valid) return x1;
    else if (y_bound(x2) && info->video_info2->is_edge_valid) return x2;
    else return 0;
}
static IsRobotSeen(Info * info) {
       return (info->video_info->is_robot_seen || info->video_info2->is_robot_seen);
}
static IsRobotAcc(Info * info) {
       return (info->video_info->is_robot_acc || info->video_info2->is_robot_acc);
}
static getRobotX(Info * info) {
    int ret = 0;
    int x1 = info->video_info->robot_acc_pos.x;
    int x2 = info->video_info2->robot_acc_pos.x;
    if (x_bound(x1) && x_bound(x2) && info->video_info->is_robot_seen && info->video_info2->is_robot_seen) ret = (x1 + x2) / 2;
    else if (x_bound(x1) && info->video_info->is_robot_seen) ret = x1;
    else if (x_bound(x2) && info->video_info2->is_robot_seen) ret = x2;
    return ret;       
}
static getRobotY(Info * info) {
    int ret = 0;
    int y1 = info->video_info->robot_acc_pos.y;
    int y2 = info->video_info2->robot_acc_pos.y;
    if (y_bound(y1) && y_bound(y2) && info->video_info->is_robot_seen && info->video_info2->is_robot_seen) ret = (y1 + y2) / 2;
    else if (y_bound(y1) && info->video_info->is_robot_seen) ret = y1;
    else if (y_bound(y2) && info->video_info2->is_robot_seen) ret = y2;
    return ret;       
}
static int IsOutSide(Info * info) {
    return (info->video_info->is_outside || info->video_info2->is_outside);
}

static void search_ball(Info * info) {
        //info->destP.x = gety(info);
        //info->destP.y = getx(info)+50;
        //do_forward(info,DEFAULT_A);
       /*if (is_start == 0 && POSITI !=0) {
            run_rota(45,POSITI);
            run_forward(50,200);
            //run_rotaR(25);
            //run_forward(40,200);
            is_start = 1;        
        }*/                              //--- by hanlei
        if (IsBallCatch(info)) {
            info->state = BALL_CATCHED;
            info->sched_var.waitlost = 3;
            info->sched_var.doorseen = 0;            
            mylogfd(SCHFD, "[sche]State changing:SEARCH_BALL->ball_catched\n");
            if (!IsDoorSeen(info)) do_rota_with_ball(info, POSITI*360, 1);
            return;
        }  
        if (IsMiddle(info)) {            
            if (info->sched_var.waitforward -- == 0) {
                    info->sched_var.waitforward = SCHED_waitforward_TIME;
                    info->last_cmd = CMD_FORWARD;
                    info->state = FORWARDING_BALL;
                       
                    info->destP.x = gety(info);                
                    info->destP.y = - getx(info);
                    mylogfd(SCHFD, "[sche]SEARCH_BALL->forwarding_ball\n");
                    do_forward(info, DEFAULT_A);
                    return;
            }
        } else  {
			info->sched_var.count = 0;
			info->sched_var.door_count = 0;
			info->sched_var.door_dis = 10000;
			info->sched_var.max_green_sum = 0;
			info->sched_var.max_green_count = 0;
            if (IsBallAllSeen(info)) info->sched_var.wait = 0;
            else if (IsBallSeen(info)) info->sched_var.wait = 1;
            else info->sched_var.wait = 3;
            int dir ;
            if (START_FLAG == 0){
                START_FLAG = 1;
                dir = POSITI;
            }           
            dir = info->sched_var.search_dir;               //  search_ball -- redwalker
            if (IsBallSeen(info) && IsRight(info)) dir = -1; 
            if (IsBallSeen(info) && IsLeft(info)) dir = 1;
            info->state = SEARCHING_BALL;
            mylogfd(SCHFD, "[sche]SEARCH_BALL->search_ball\n");
            info->last_cmd = CMD_SEARCH_BALL;
            info->sched_var.search_dir = dir;
            do_search_ball(info, dir);
        }
}
static int getGreenSum(Info * info) {
		return (info->video_info->green_sum + info->video_info2->green_sum) / 2;
}
static void searching_ball(Info * info) {
		info->sched_var.count ++;
		//mylogfd(SCHFD, "count = %d\n", info->sched_var.count);
		if (IsDoorSeen(info)) {
			if (getDoorMidx(info) != 0 && getDoorMidx(info) < info->sched_var.door_dis) {
				info->sched_var.door_dis = getDoorMidx(info);
				info->sched_var.door_count = info->sched_var.count;
			}
		}
		if (getGreenSum(info) > info->sched_var.max_green_sum) {
			info->sched_var.max_green_sum = getGreenSum(info);
			info->sched_var.max_green_count = info->sched_var.count;
		}
        if (IsBallSeen(info))  {	
            mylogfd(SCHFD, "[sche]search_ball->forwarding_ball\n");
            info->last_cmd = CMD_FORWARD;
            info->state = FORWARDING_BALL;
            mylogfd(SCHFD,"[sche]left %d %d %d %d, right %d %d %d %d\n", info->video_info->is_ball_seen, info->video_info->is_ball_acc, info->video_info->ball_acc_pos.x, info->video_info->ball_acc_pos.y,
                                                        info->video_info2->is_ball_seen, info->video_info2->is_ball_acc, info->video_info2->ball_acc_pos.x, info->video_info2->ball_acc_pos.y);
            info->destP.x = gety(info);
            info->destP.y = - getx(info);
            if (!info->video_info->is_ball_acc && !info->video_info2->is_ball_acc) {
               info->destP.x = 100;
               info->destP.y = 0;
            }
            do_forward(info, DEFAULT_A);
            return;
        }
        if (info->sched_var.count >= 90 || checkDone()) {
            mylogfd(SCHFD, "[sche]searching_ball->change_place checkdone\n");
            info->state = CHANGE_PLACE;
            do_search_ball(info, info->sched_var.search_dir);
            return;
        }
}
static void searching_adjust(Info * info) {
        if (IsMiddle(info)) {
            mylogfd(SCHFD, "[sche]searching_adjust->forwarding_ball\n");
            info->last_cmd = CMD_STOP;
            info->state = FORWARDING_BALL;
            do_stop(info);
            return;
        } if (checkDone()) {
            mylogfd(SCHFD, "[sche]checkDone searching_adjust\n");
            mylogfd(SCHFD, "[sche]searching_adjust->SEARCH_BALL\n");
            //info->last_cmd = CMD_STOP;
            info->state = SEARCH_BALL;
            // do_stop(info);
            return;
        }
}
static void forwarding_ball(Info * info) {
        if (IsRight(info)) info->sched_var.search_dir = -2;
        if (IsLeft(info)) info->sched_var.search_dir = 2;
        if (IsBallCatch(info))   {
            mylogfd(SCHFD, "[sche]forwarding_ball->ball_catched\n");
            info->last_cmd = CMD_STOP; 
            info->state = BALL_CATCHED;
            info->sched_var.doorseen = 0;        
            do_stop(info);            //  delet // -- redwalker
            if (!IsDoorSeen(info)) do_rota_with_ball(info, POSITI*360, 1);
           return;
        }
        if (!IsBallSeen(info)) {  
            mylogfd(SCHFD, "[sche]forwarding_ball->stop when lose ball\n");
            info->state = SEARCH_BALL;
            do_stop(info);
            return;
        }
       /* else if (IsBallVeryNear(info))
        {
            mylogfd(SCHFD, "[sche]catching_ball->forwarding_ball\n");
            info->state = CATCHING_BALL;
            info->destP.x = gety(info);
            iinfo->destP.y = -getx(info);
            do_catch_ball(info);
        }*/
        if (IsMiddle(info)) {
            do_adjust(-1);
        }
        if (IsLeft(info)) {
            do_adjust(1);
        }
        if (IsRight(info)) {
            do_adjust(0);
        }
        if (checkDone ()) {
            mylogfd(SCHFD, "[sche]checkDone forwarding_ball\n");
            info->state = SEARCH_BALL;
            mylogfd(SCHFD, "[sche]forwarding_ball->SEARCH_BALL\n");
            return;
        }         
}
static void catching_ball(Info* info) {
        if (IsBallCatch(info)) {
            mylogfd(SCHFD, "[sche]State changing:catching_ball->ball_catched\n"); 
            info->state = BALL_CATCHED;
            info->sched_var.doorseen = 0;
            info->sched_var.waitlost = 3;       
            do_stop(info);
            if (!IsDoorSeen(info)) do_rota_with_ball(info, -POSITI*360, 1);    // get in a paramiter -- redwalker

        }
        else if (!IsBallAllSeen(info)) {
             mylogfd(SCHFD, "[sche]catching_ball->SEARCH_BALL\n");
             info->state = SEARCH_BALL;
             do_stop(info);
        }
        else if (!IsBallVeryNear(info))
        {
            mylogfd(SCHFD, "[sche]catching_ball->forwarding_ball\n");
            info->state = FORWARDING_BALL;
            info->destP.x = gety(info);
            info->destP.y = - getx(info);
            do_forward(info, DEFAULT_A);
        }
        else if (checkDone()) {
            mylogfd(SCHFD,"[sche]checkDone catching_ball \n");
            info->last_cmd = CMD_CATCH_BALL;
            info->destP.x = gety(info);
            info->destP.y = - getx(info); 
            do_catch_ball(info);
        }
        if (IsMiddle(info)) {
            do_adjust(-1);
        }
        if (IsLeft(info)) {
            do_adjust(1);
        }
        if (IsRight(info)) {
            do_adjust(0);
        }
}
static void ball_catched(Info* info) {
        if (IsRight(info)) info->sched_var.search_dir = -1;
        if (IsLeft(info)) info->sched_var.search_dir = 1;
        if (IsDoorSeen(info)) {
            info->sched_var.doorseen = 1;
        }

     /*   if (info->sched_var.doorseen && !IsDoorSeen(info)) {
            info->sched_var.doorseen = 0;
            info->state = MOVING;
            do_rota_with_ball(info, 90, 1);
            return;
        }*/
     //   mylogfd(SCHFD, "%d %d %d\n", info->sched_var.is_door_seen, info->video_info->door_pos.x, info->video_info2->door_pos.x);

        if (IsDoorSeen(info)) {
			if (getDoorMidy(info) > 250) {
				info->destP.x = 50;
				info->destP.y = 0;
				info->state = MOVING;
				do_catch_ball(info);
			}
            mylogfd(SCHFD, "[sche]see door to face door\n");
			if (IsDoorLeft(info)) info->sched_var.search_door_dir = 1;
			else info->sched_var.search_door_dir = -1;
		//	do_stop(info);
			do_rota_with_ball(info, info->sched_var.search_door_dir * 90, 1);
            info->state = FACE_DOOR;
            return;
        }
        if (!IsBallCatch(info)) {
            mylogfd(SCHFD, "[sche]lose balll\n");
            info->last_cmd = CMD_STOP;
            do_stop(info);
            info->state = SEARCH_BALL;
            return;
        }
}
static void face_door(Info * info) {	
		if (!IsBallCatch(info)) {
			mylogfd(SCHFD, "[sche]lose ball\n");
			info->state = SEARCH_BALL;
			do_stop(info);
			return;
		}
		if (getDoorRightx(info) * getDoorMidx(info) < 0) {
			int cx = 0;
			int x1 = getDoorMidx(info);
			int y1 = getDoorMidy(info);
			int x2 = getDoorRightx(info);
			int y2 = getDoorRighty(info);
			int cy = abs(x1) * y1 / (abs(x1) + abs(x2)) + abs(x2) * y2 / (abs(x1) + abs(x2));
			int d1 = Distance(getDoorRightx(info), getDoorRighty(info), getRobotX(info), getRobotY(info));
			int d2 = Distance(getRobotX(info), getRobotY(info), cx, cy);
			int d = Distance(x2, y2, cx, cy);
mylogfd(SCHFD, "right x1 = %d y1 = %d x2 = %d y2 = %d cy = %d d1 = %d d2 = %d d = %d %d\n", x1, y1, x2, y2, cy, d1, d2, d, getRobotX(info));
			mylogfd(SCHFD, "robot=%d door=%d\n", getRobotX(info), getDoorMidx(info));
			if (!IsRobotSeen(info) || getRobotX(info) <= getDoorMidx(info)) {			
			//	do_stop(info);
				info->state = SHOOTING;
				info->destP.x = cy + 100;
				info->destP.y = - getDoorRightx(info) + 5;
				do_forward(info, MAX_A);
				return;
			}
		}
		if (getDoorLeftx(info) * getDoorMidx(info) < 0) {
			int cx = 0;
			int x1 = getDoorMidx(info);
			int y1 = getDoorMidy(info);
			int x2 = getDoorLeftx(info);
			int y2 = getDoorLefty(info);
			int cy = abs(x1) * y1 / (abs(x1) + abs(x2)) + abs(x2) * y2 / (abs(x1) + abs(x2));
			int d1 = Distance(getDoorLeftx(info), getDoorLefty(info), getRobotX(info), getRobotY(info));
			int d2 = Distance(getRobotX(info), getRobotY(info), cx, cy);
			int d = Distance(x2, y2, cx, cy);
mylogfd(SCHFD, "left:x1 = %d y1 = %d x2 = %d y2 = %d cy = %d d1 = %d d2 = %d d = %d %d\n", x1, y1, x2, y2, cy, d1, d2, d, getRobotX(info));
			mylogfd(SCHFD, "robot=%d door = %d\n", getRobotX(info), getDoorMidx(info));
			if (!IsRobotSeen(info) || getRobotX(info) >= getDoorMidx(info)) {
				info->state = SHOOTING;
				//do_stop(info);
				info->state = SHOOTING;
				info->destP.x = cy + 100;
				info->destP.y = - getDoorLeftx(info) - 5;
				do_forward(info, MAX_A);             // adjust while shooting -- redwalker
				if (!IsBallCatch(info)) {                            
					mylogfd(SCHFD, "[sche]lose ball\n");
					info->state = SEARCH_BALL;
					do_stop(info);
					return;
				}
		if (IsDoorMiddle(info)) {
            do_adjust(-1);
        }
        if (IsDoorLeft(info)) {
            do_adjust(1);
        }
        if (IsDoorRight(info)) {
            do_adjust(0);
        }				
				return;			
			}
		}
		if (checkDone()) {
            mylogfd(SCHFD,"[sche]checkDone face_door\n");
			info->sched_var.search_door_dir = -info->sched_var.search_door_dir;
			do_rota_with_ball(info, 90 * info->sched_var.search_door_dir, 1);
			return;
		}
}
static void shoot (Info * info) {
     //   mylogfd(SCHFD, "[sche]shoot %d\n", info->video_info->door_pos.y);
     
        info->destP.x = 300;
      //  info->destP.y = - getDoorMidx(info) + 100 * (- getDoorMidx(info)) / getDoorMidy(info);
//mylogfd(SCHFD, "[sche shoot]left eye %d %d %d, right eye %d %d %d\n", info->video_info->is_door_seen, info->video_info->door_pos.x, info->video_info->door_pos.y,
  //                                                                     info->video_info2->is_door_seen, info->video_info2->door_pos.x, info->video_info2->door_pos.y);
        do_forward(info, MAX_A); 
        info->state = SHOOTING;
}
static void shooting(Info* info) {
       if (checkDone()) {
            mylogfd(SCHFD, "[sche]checkDone shooting\n");
            mylogfd(SCHFD, "[sche]shooted\n");
		//	do_adjust(-1);
            do_stop(info);
//            info->destP.x = -100;
  //          info->destP.y = 0;
//            do_forward(info, DEFAULT_A);
            info->state = SHOOTED;        
            return;
        }
       mylogfd(SCHFD, "shooting green %d\n", (info->video_info->green_sum + info->video_info->green_sum) / 2);
        if ((info->video_info->green_sum + info->video_info->green_sum) / 2 < 3500) {
			mylogfd(SCHFD, "[sche]door arrived then shooted\n");
			do_stop(info);
			info->state = SHOOTED;
			return;
		}
		if (IsDoorLeft(info)) {
           do_adjust(1);
        } else if (IsDoorRight(info)) {
           do_adjust(0);
        } else if (IsDoorMiddle(info)) {
           do_adjust(-1);
        }
        if (!IsBallCatch(info)) {
			mylogfd(SCHFD, "shooting->search_ball when lose ball\n");
            do_stop(info);
            info->state = SEARCH_BALL;
 		}	
        if (!IsDoorSeen(info)) {
            mylogfd(SCHFD, "shooting->ball_catched when lose door %d %d\n", info->video_info->is_door_seen, info->video_info2->is_door_seen);
            do_stop(info);
            info->state = BALL_CATCHED;
        }  
}
static void moving(Info * info) {
    if (checkDone()) {
        mylogfd(SCHFD, "[sche]checkDone moving\n");
		info->state = FACE_DOOR;
		return;
	}
	if (!IsBallCatch(info)) {
		do_stop(info);
		info->state = SEARCH_BALL;
		return;
	}
    mylogfd(SCHFD,"MOVING");
    /*static int step = 0;
    if (step == 1 && checkDone()) {
        step = 0;
        info->state = BALL_CATCHED;
        do_rota_with_ball(info, 360, 1);
        return;    
    }    
    if (step == 0 && checkDone()) {
        step = 1;
        info->destP.y = 70;
        info->destP.x = 0;  
        do_catch_ball(info);
        return;
    }*/
}
static void change_place(Info * info) {
mylogfd(SCHFD, "green count = %d\n", info->sched_var.max_green_count);
	if (info->sched_var.max_green_count-- == 0) {
		info->state = CHANGING_PLACE;
		info->destP.x = 100;
		info->destP.y = 0;
		if (info->sched_var.door_dis > 250) info->destP.x = 50;
		do_forward(info, DEFAULT_A);
		return;
	}    
}
static void finding_dir(Info * info) {
//mylogfd(SCHFD, "[sche]finding_dir:%d %d\n", IsEdgeValid(info), getEdgeX(info));
    if (IsEdgeValid(info) && (!IsOutSide(info) || getEdgeX(info) > 100)) {
        info->destP.x = 50 + Random(200);
        if (info->destP.x > getEdgeX(info) - 10) info->destP.x = getEdgeX(info) - 10;
        info->destP.y = 0; 
        info->state = CHANGING_PLACE; 
        do_stop(info);      
        do_forward(info, MAX_A);
        mylogfd(SCHFD, "[sche]finding_dir->changing_place");
    }
}
static void changing_place(Info * info) {
    if (checkDone()) {
        mylogfd(SCHFD, "[sche]checkDone changing_place \n");       
        info->state = SEARCH_BALL;
        mylogfd(SCHFD, "[sche]changing_place->search_ball\n");   
    }
}
static void start(Info * info){
	mylogfd(SCHFD, "[sche]start\n");
    info->destP.x = gety(info)+100;
    info->destP.y = getx(info)+POSITI*200;
    do_run(info);
    if(IsBallSeen(info)) info->state = FORWARDING_BALL;
    if(checkDone()) info->state = SEARCH_BALL;
    /* if(checkDone()) { 
    mylogfd(SCHFD, "[sche]rotate_end\n");
                 //  do_forward -- redwalker
    if(checkDone()){
        info->state = SEARCH_BALL;
        mylogfd(SCHFD,"[sche]start->search_ball\n");
    }*/
}
/*static void find_place(Info * info) {                  // delet find_place -- redwalker
    static int step = 0;
    static int laststep = -1;
    if (step != laststep) {
        mylogfd(SCHFD, "step = %d\n", step);
        laststep = step;
    }
    if (step == 0) {
       do_rota(info, 360, 1);   // get in a para-- redwalker
       step = 1;
    } else if (step == 1) {
       if (IsDoorSeen(info) && IsDoorMiddle(info)) {
             //mylogfd(SCHFD, "Door Seen\n");
             if (IsBallSeen(info)) {
                 do_stop(info);
                 step = 0;
                 info->state = SEARCH_BALL;
                 return;
             }
             do_stop(info);
             do_rota(info, 180, 1);   // get in a para -- redwalker
             step = 2;
       }            
    } else if (step == 2) {
       if (IsBallSeen(info)) {
          if (IsDoorSeen(info)) {
             do_stop(info);
             step = 0;
             info->state = SEARCH_BALL;
             return;
          }
         info->destP.x = gety(info) + 50;
         info->destP.y = - getx(info);
         info->obstacle[0].x = gety(info);
         info->obstacle[0].y = - getx(info);
         do_obstacle(info);
         step = 4;
       }
       if (checkDone()) {
          do_rota(info, 180, 1);           // get in para -- redwalker
          step = 3;
       }
    } else if (step == 3) {
       if (IsBallSeen(info)) {
          if (IsDoorSeen(info)) {
             do_stop(info);
             step = 0;
             info->state = SEARCH_BALL;
             return;
          }
         info->destP.x = gety(info) + 50;
         info->destP.y = - getx(info);
         info->obstacle[0].x = gety(info);
         info->obstacle[0].y = - getx(info);
         do_obstacle(info);
         step = 4;
       }
       if (checkDone()) {
          do_rota(info, 180, 1);           // get in a para -- redwalker
          step = 3;
       }
    } else if (step == 4) {
      if (checkDone()) {
         step = 5;
      }   
    } else if (step == 5) {
      if (checkDone()) {
         info->state = SEARCH_BALL;
      }
    }
}
*/
void info_init(Info * info) {
	info->state = START;
	info->last_cmd = CMD_NO;
	info->robotP.x = 0;
	info->robotP.y = 0;
	info->direction = 0;
	info->sched_var.door_dir = 360;
	info->sched_var.is_ball_seen = 0;
	info->sched_var.is_door_seen = 0;
	info->sched_var.doorseen = 0;    
    int i = 0;
    for (i = 0; i < 3; i++) 
        info->obstacle[i] = UNKNOW;
// UNKNOW;
    info->sched_var.wait = SCHED_WAIT_TIME;
	info->sched_var.waitforward = SCHED_WAIT_TIME;
	info->sched_var.search_dir = 1;
    srand((unsigned int)time(NULL));
    mylogfd(SCHFD,"[sche]INFO_INIT");
}
void estimate_seen_state(Info * info) {
    static int ball_seen_wait = 0;
	if (info->video_info->is_ball_seen || info->video_info2->is_ball_seen) {
		ball_seen_wait ++;
		if (ball_seen_wait >= 3 || info->sched_var.is_ball_seen) {
			info->sched_var.is_ball_seen = 1;
			ball_seen_wait = 3;
		}
	} else {
		ball_seen_wait --;
		if (ball_seen_wait <= 0 || info->sched_var.is_ball_seen == 0) {
			info->sched_var.is_ball_seen = 0;
			ball_seen_wait = 0;
		}
	}
	
	static int door_seen_wait = 0;
//mylogfd(SCHFD, "door_seen_wait %d\n", door_seen_wait);
	if (info->video_info->is_door_seen || info->video_info2->is_door_seen) {
		door_seen_wait ++;
		if (door_seen_wait >= 3 || info->sched_var.is_door_seen) {
			info->sched_var.is_door_seen = 1;
			door_seen_wait = 3;
		}
	} else {
		door_seen_wait --;
		if (door_seen_wait <= 0 || info->sched_var.is_door_seen == 0) {
			info->sched_var.is_door_seen = 0;
			door_seen_wait = 0;
		}
	}
}
static void obstacle(Info * info) {
       if (checkDone()) {
          info->state = SEARCH_BALL;
       }
}
static void protect_door(Info * info) {
       static int angle = 180;
       if (IsBallSeen(info)) {
          do_stop(info);
       
          
       }
       if (checkDone()) {
          do_rota(info, angle, 1);
          angle = -angle;          
       }
}
/*static void game(Info * info) {                  // delet game -- redwalker

	static int step = 0;
	if (checkDone()) {
		mylogfd(SCHFD, "SEARCH_BALL\n");
		info->state = SEARCH_BALL;
		step = 0;
		return;
	}	
	if (step == 0) {
		if (IsBallSeen(info)) step = 1;
	}
	else {
		if (!IsBallSeen(info)) {
			info->state = SEARCH_BALL;
			do_adjust(-1);
			do_stop(info);
			step = 0;
			return;
		}
		if (IsMiddle(info)) {
            do_adjust(-1);
        }
        if (IsLeft(info)) {
            do_adjust(1);
        }
        if (IsRight(info)) {
            do_adjust(0);
        }
	}
}
*/
static void shooted(Info * info) {
}
void sched (Info * info) {
    //mylogfd (SCHFD, "%d\n", info->state);
//    if (!info->video_info->is_valid) return;
//    mylogfd(SCHFD, "%d %d\n", IsEdgeValid(info),  getEdgeX(info));
  //  mylogfd(SCHFD, "IsDoorSeen %d IsDoorAllSeen %d IsDoorLeft %d IsDoorRight %d IsDoorMiddle %d DoorMidX %d DoorMidY %d\n",
	//				IsDoorSeen(info), IsDoorAllSeen(info), IsDoorLeft(info), IsDoorRight(info), IsDoorMiddle(info), getDoorMidx(info), getDoorMidy(info));
    /*static int wait_edge =  -1;
    if (wait_edge >= 0) {
		wait_edge ++;
		if (wait_edge >= 5) {
			mylogfd(SCHFD, "Touch edges->Stop\n");
			do_stop(info);
			wait_edge = -1;
		}
		return;
	}*/
  // mylogfd(SCHFD, "IsDoorSeen:%d IsEdgeValid:%d IsOutSide:%d edgeX %d\n", IsDoorAllSeen(info), IsEdgeValid(info), IsOutSide(info), getEdgeX(info)); 	
   estimate_seen_state(info);
  // mylogfd(SCHFD, "IsRobotSeen %d RobotX %d RobotY %d\n", IsRobotSeen(info), getRobotX(info), getRobotY(info));
   if (info->last_cmd == CMD_FORWARD && info->state != SHOOTING && IsOutSide(info)) {
		mylogfd(SCHFD, "Outside\n");
		info->last_cmd = CMD_STOP;
		do_stop(info);
		info->state = OUTSIDE;
		return;
   }
  /* if (info->last_cmd == CMD_FORWARD && IsOutSide(info) && IsEdgeValid(info)) {
        if (getEdgeX(info) < 50 && getEdgeX(info) >= 0) {
            mylogfd(SCHFD, "Touch edges %d\n", getEdgeX(info));            
            do_stop(info);
			info->state = DONE;
            //wait_edge = 0;
            return;        
        }    
    }*/
/*   	if (info->last_cmd == CMD_FORWARD && IsRobotSeen(info)) {
       if (IsRobotAcc(info)) {
          if (getRobotY(info) < 30) {
             do_stop(info);
             info->state = SEARCH_BALL;
          } else if (getRobotY(info) < 60) {
             info->destP.x = 0;
             info->destP.y = getRobotY(info) + 50;
             info->obstacle[0].x = getRobotY(info);
             info->obstacle[0].y = - getRobotX(info);
             do_obstacle(info);
             info->state = OBSTACLE;
          }
       }
    }*/
    //mylogfd(-1, "s");
	switch (info->state) {
	case START:                           //chang in state machine -- redwalker
		start(info);
    case FIND_PLACE:
    //    find_place(info);
        break;
    case SEARCH_BALL:
		search_ball(info);
        break;
    case SEARCHING_BALL:
        searching_ball(info);
        break;
    case SEARCHING_ADJUST:
        searching_adjust(info);
        break;
    case FORWARDING_BALL:
        forwarding_ball(info);    
        break;
    case CATCHING_BALL:
        catching_ball(info);
        break;
    case BALL_CATCHED:
        ball_catched(info);        
        break;
    case SHOOT:
        shoot(info);
        break;
    case SHOOTING:
        shooting(info);  
        break;
    case MOVING:
        moving(info);
        break;
    case CHANGE_PLACE:
        change_place(info);
        break;
    case FINDING_DIR:
        finding_dir(info);
        break;
    case CHANGING_PLACE:
        changing_place(info);
        break;
    case OBSTACLE:
         obstacle(info);
         break;
    case PROTECT_DOOR:
         protect_door(info);
         break;
    case OUTSIDE:
		 if (!IsOutSide(info)) {
			mylogfd(SCHFD, "Inside restart to search_ball\n");
			info->state = SEARCH_BALL;
		 }
		 break;
	case FACE_DOOR:
		face_door(info);
		break;
    case DONE:
        break;
	case GAME:
		//game(info);
		break;
	case SHOOTED:
		shooted(info);
    default :
        break;
	}
}
