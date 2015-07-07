#ifndef GLOBAL_H
#define GLOBAL_H

 
#define W 320
#define H 240
/*
#define W 640 
#define H 480 
*/
#define R 30

#define RAW_SIZE 19440
#define MODE 136
#define BANDLEN 324

#define W_2 (W/2)
#define H_2 (H/2)

#define Y_SIZE (W*H)
#define U_SIZE (W*H/4)
#define YUV_SIZE (W*H*3/2)

#define INVALID_BALL_Y 40
#define BOTTOM_LINE 116
#define BALL_NEAR_K 2
#define BALL_NEAR_X 47
#define BALL_CENTER_OFFSET 10
#define BALL_NEAR_RIGHT 130
#define BALL_NEAR_LEFT 145
#define DISTANCE_FAR 120
#define DISTANCE_NEAR 50
#define DISTANCE_VERY_FAR 200
#define TURN_OFFSET_FAR 10

#define COLOR_B 0 // ball
#define COLOR_G 1 // goal
#define COLOR_S 2//self door
#define COLOR_C 3 // court
#define COLOR_L 4 // line

// add to find robot
#define COLOR_R 5 // robot

#define ROBOT_OFFSET 5
// add end

#define restrict __restrict__

#endif

