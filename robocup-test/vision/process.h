#ifndef PROCESS_H
#define PROCESS_H

#include "cmv.h"
#include "v4l2cap.h"
#include "../my_utils.h"


#define DEFAULT_DOOR_NEAR_THRESHOLD 110

#define MESSAGE_NUMBER 10

extern int hgoal, goal_x, goal_y;
extern int hball, ball_x, ball_y;
extern int can_see_green_carpet;

extern struct cmv_data data[2];
void process(struct vd_capture* vd, int i);
//void process(unsigned char *src, int size, int i);
void process_init();
int out_info();
#endif
