#ifndef CMV_H
#define CMV_H

#include "global.h"


#define COLOR_DEPTH 256
#define INT_BITS 32
#define PI 3.1415926
#define MAX_LINES 10

#define LOC_LEFT 2000
#define LOC_RIGHT -2000
#define LOC_INVALID -10000
#define LOC_CATCH_BALL 10000
#define LOC_Y_FAR    300
#define LOC_Y_INVALID 9999
#define LOC_Y_NEAR   1000
#define LOC_Y_VERY_NEAR 500
#define DOOR_LENGTH_2 60
#define DOOR_OFFSET 5

#define PAST_LEN 5
// type of run_length_encode
struct rle_t {
    int color;
    int x1, x2;
    void *parent;
};

// type of color_region
struct reg_t {
    int color;
    int size;
    int x1, x2, y1, y2;	//外包矩形对角线的坐标
    int cx, cy;		//外包矩形中被标记为某种颜色所有点的中心坐标
};

struct cmv_data {
    int color_n;
    int (*th)[6];
    int thresh[3][COLOR_DEPTH];
    int map[U_SIZE + H_2];
    int *remap;

    int rle_max;
    struct rle_t *rle, *rle_end;
    int reg_max;
    struct reg_t *reg, *reg_end;
    int size_min;	//区域的最小面积

    int (*rgb)[3];
    int rgb_n;

    int *clist;
    struct reg_t **cmax;
    //struct reg_t **cmv_ctable; // color_n, MAX_RPC
};

struct spot {
    int x;
    int y;
};
typedef struct door_info {
    int is_door_seen;
    struct spot door_pos;
    struct spot door_acc_pos;

    int is_door_right;
    struct spot door_right;
    int is_door_left;
    struct spot door_left;
} door_info;

struct local_parse_info {
    double vparam[8];//translate matrix

    int current_count;
    //struct local_parse_info past_state[PAST_LEN];
    //int is_ps_valid[PAST_LEN];
    door_info enemy_door;
    door_info mine_door;

    //ball info
    struct spot ball_center;
    int ball_radius;

    int is_ball_acc;//是否可以精确定位球
    struct spot ball_acc_pos;//球的精确位置

    int is_ball_seen;//是否可以看见球
    struct spot ball_pos;//大概位置

    int is_robot_seen;
    struct spot robot_pos;
    int is_robot_acc;
    struct spot robot_acc_pos;
    int green_sum;
    int is_outside;
};
#ifdef __cplusplus
extern "C" {
#endif

    int ball_filter(struct reg_t * g,struct cmv_data * data);
    int ball_outline_detection(struct cmv_data *data, int* x, int* y, int* radius);
    void cmv_init(struct cmv_data * restrict data, const char * restrict filename);
    void cmv_process(struct cmv_data * restrict data, const unsigned char * restrict src);
    void cmv_reinit(struct cmv_data * restrict data, int ch, int i, int target, int type);
    void cmv_save(const struct cmv_data * data, const char *filename);
    void cmv_reset_thresh(struct cmv_data *data, int c, int *v);
    int goal_detection(struct cmv_data *data, unsigned char* src, struct spot * left, struct spot * right, int* isLeftArrived, int* isRightArrived, struct local_parse_info * local_tmp,struct local_parse_info ** data_t,int color_tmp);
    int edge_detection(struct cmv_data *data, double * angle, int * radius, int * count);
    void getxy(double u, double v, int *x, int *y, double param[]);
    int catchball(struct reg_t *reg);
    int find_ball(int num, struct cmv_data * data_tmp,struct local_parse_info * info_tmp);
    void getEdgeDistance(struct spot left, struct spot right, int * dis, double * angle, struct local_parse_info * local_tmp);
    int left_right(int x, int y);
    void translate_init(struct local_parse_info * local_info);
    int find_door(int num, struct cmv_data * data_tmp, struct local_parse_info *local_tmp,unsigned char * yuv_buf,int is_mine_door);
    int find_robot(int num, struct cmv_data * data_tmp,struct local_parse_info * info_tmp);
    void judge_outside(struct cmv_data * data, struct local_parse_info *local_tmp);
#ifdef __cplusplus
}
#endif

#endif

