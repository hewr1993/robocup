#ifndef PROCESS_H
#define PROCESS_H

#include "cmv.h"

#define LOC_LEFT 2000
#define LOC_RIGHT -2000
#define LOC_INVALID -10000
#define LOC_CATCH_BALL 10000
#define LOC_Y_FAR    2000
#define LOC_Y_INVALID 9999
#define LOC_Y_NEAR   1000
#define LOC_Y_VERY_NEAR 500

#define COLOR_B 0
#define COLOR_G 1


#define DEFAULT_DOOR_NEAR_THRESHOLD 18

extern int hgoal, goal_x, goal_y;
extern int hball, ball_x, ball_y;
extern int can_see_green_carpet;

extern struct cmv_data data[2];
void process(unsigned char *src, int size, int i);
void process_init();

#endif
