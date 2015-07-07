#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "cmv.h"
#include "process.h"
#include "utils.h"
#include "global.h"
#include "dec.h"

unsigned char yuv_buf[YUV_SIZE];
struct cmv_data data[2];
struct local_parse_info local_info[2];

int edge_distance[2], edge_angle[2];

const char *cfilename0 = "colors0.txt";
const char *cfilename1 = "colors1.txt";

static int get_info(int i);
//static int is_ball_valid(struct reg_t *reg);

int door_near_threshold = DEFAULT_DOOR_NEAR_THRESHOLD;

static int *pthreshold[] = {
    &door_near_threshold,
};

static int len_pthreshold = 1;
void process(struct vd_capture* vd, int num)
{
    if (vd->format == V4L2_PIX_FMT_MJPEG) {
        unsigned char *tmp=(unsigned char*)calloc(1, (size_t)((vd->width*vd->height)<<1));
        if (!tmp) {
            mylogfd(2, "[Version|Pro|process] Error occurs when calloc\n");
            return;
        }
        jpeg_decode(&tmp, vd->framebuffer, (int*)&vd->width, (int*)&vd->height);
        yuv422to420p(tmp, yuv_buf, vd->width, vd->height);
        free(tmp);
    } else
        yuv422to420p(vd->framebuffer, yuv_buf, vd->width, vd->height);

    //use cmv to cut the image
    cmv_process(&data[num], yuv_buf);
    //TODO We havn't  judged outside, but we will

    get_info(num);
    //out_info(num);
}

/*
	just read the data from files: such as
	color0.txt, color1.txt, translate.conf, video.conf
*/
void process_init()
{

    dec_init();

    cmv_init(&data[0], cfilename0);
    cmv_init(&data[1], cfilename1);

    translate_init(local_info);

    FILE* fp = fopen("video.conf", "r");
    if (fp == NULL) {
        mylogfd(2,"[Version|Pro|pro_init] Read video.conf failed1\n");
	perror("1:\n");
        exit(-1);
    }
    char buf[256];
    int temp_input,count=0;

    while (!feof(fp)) {
        if (!fgets(buf, 256, fp))
            mylogfd(2,"[Version|Pro|pro_init] Read file failed2\n");
        if (strlen(buf) < 2)
            continue;
        sscanf(buf, "%d", &temp_input);
        *pthreshold[count++] = temp_input;
        if (count >= len_pthreshold)
            break;
    }

    mylogfd(logfd, "[Version|Pro|pro_init] Door_near_threshold %d\n", door_near_threshold);
    fclose(fp);
}

int get_info(int num)
{
    find_ball(num,&(data[num]),&(local_info[num]));
//    fprintf (stderr, "[1]");
    find_door(num,&(data[num]),&(local_info[num]),yuv_buf,0);
//fprintf (stderr, "[2]");
    find_door(num,&(data[num]),&(local_info[num]),yuv_buf,1);
//fprintf (stderr, "[3]");
    judge_outside(&(data[num]),&(local_info[num]));
//fprintf (stderr, "[4]");
    find_robot(num,&(data[num]),&(local_info[num]));
//fprintf (stderr, "[5]");

    double angle[MAX_LINES];
    int radius[MAX_LINES];
    int count;
    double far = 1000;
    edge_detection(&data[0], angle, radius, &count);
    struct spot e_l, e_r;
    int j=0;
    double tmp_ang;
    int tmp_dis;
    edge_angle[num] = 180;
    edge_distance[num] = 200;
    for (; j<count; j++) {
        if (abs(angle[j])<3) {
            e_l.x = 0;
            e_l.y = 2*(H_2 - 1 - (int)(radius[j] / sin(angle[j])));
            e_r.x = 2*(W_2 - 1);
            e_r.y = 2*(H_2 - 1 - (int)((radius[j] - (W_2 - 1)*cos(angle[j])) / sin(angle[j])));
        } else {
            e_l.x = 2*(int)(radius[j] / cos(angle[j]));
            e_l.y = 2*(H_2 - 1 );
            e_r.x = 2*(int)((radius[j] - (H_2 - 1)*sin(angle[j])) / cos(angle[j]));
            e_r.y = 2;
        }
        getEdgeDistance(e_l, e_r, &tmp_dis, &tmp_ang,&(local_info[num]));
        double tmp_far = fabs(tmp_dis) * fabs(cos(tmp_ang));
        int tmp_angle = (int)(tmp_ang * 180 / PI);
        if (((abs(tmp_angle - edge_angle[num]) < 5 && tmp_far < far)
                || abs(tmp_angle) < abs(edge_angle[num]) -5)&&tmp_dis>=0&&tmp_dis<=500) {
            edge_distance[num] = (int)tmp_dis;
            edge_angle[num] = tmp_angle;
            far = tmp_far;
        }
    }
//fprintf (stderr, "[6]");
fflush (stderr);
    //}
    return 0;
}

int out_info()
{
    int i=0;
    //mylogfd(1, "befor\n");
    printf("vido ");
    for (i=0; i<2; i++) {
        //mylogfd(1, "infor\n");
        printf("n %d # ", i);
        if (local_info[i].is_ball_seen)
            printf("b %d %d # ", local_info[i].ball_pos.x, local_info[i].ball_pos.y);
        else
            printf("b -10000 # ");
        if (local_info[i].enemy_door.is_door_seen)
            printf("d %d %d # ", local_info[i].enemy_door.door_pos.x, local_info[i].enemy_door.door_pos.y);
        else
            printf("d -10000 # ");
        if (local_info[i].enemy_door.is_door_left)
            printf("l %d %d # ", local_info[i].enemy_door.door_left.x, local_info[i].enemy_door.door_left.y);
        else
            printf("l -10000 # ");
        if (local_info[i].enemy_door.is_door_right)
            printf("r %d %d # ", local_info[i].enemy_door.door_right.x, local_info[i].enemy_door.door_right.y);
        else
            printf("r -10000 # ");

        if (local_info[i].mine_door.is_door_seen)
            printf("m %d %d # ", local_info[i].mine_door.door_pos.x, local_info[i].mine_door.door_pos.y);
        else
            printf("m -10000 # ");
        if (local_info[i].mine_door.is_door_left)
            printf("v %d %d # ", local_info[i].mine_door.door_left.x, local_info[i].mine_door.door_left.y);
        else
            printf("v -10000 # ");
        if (local_info[i].mine_door.is_door_right)
            printf("z %d %d # ", local_info[i].mine_door.door_right.x, local_info[i].mine_door.door_right.y);
        else
            printf("z -10000 # ");

        if (local_info[i].is_ball_acc)
            printf("p %d %d # ", local_info[i].ball_acc_pos.x, local_info[i].ball_acc_pos.y);
        else
            printf("p -10000 # ");
        if (edge_angle[i] != 180)
            printf("e %d %d # ", edge_distance[i], edge_angle[i]);
        else
            printf("e -10000 # ");
        if (local_info[i].is_robot_seen)
            printf("o %d %d # ", local_info[i].robot_pos.x, local_info[i].robot_pos.y);
        else
            printf("o -10000 # ");
        if (local_info[i].is_robot_acc)
            printf("a %d %d # ", local_info[i].robot_acc_pos.x, local_info[i].robot_acc_pos.y);
        else
            printf("a -10000 # ");

        //mylogfd(1, "88infor\n");

        printf("c %d # ", local_info[i].is_outside);
        printf("g %d # ", local_info[i].green_sum);
        printf("f %d %d # ", local_info[i].enemy_door.door_acc_pos.x,local_info[i].enemy_door.door_acc_pos.y);
        printf("h %d %d # ", local_info[i].mine_door.door_acc_pos.x,local_info[i].mine_door.door_acc_pos.y);
    }
    //mylogfd(1, "afor\n");
    printf("\n");
    //add, about another robot
    /*
    if(robot_locv)
    	printf("o %d %d # ", robot_locx, robot_locy);
    else
    	printf("o -10000 # ");
    if(robot_leftv)
    	printf("w %d %d # ", robot_leftx, robot_lefty);
    else
    	printf("w -10000 # ");
    if(robot_rightv)
    	printf("y %d %d # ", robot_rightx, robot_righty);
    else
    	printf("y -10000 # ");
    //add end
    printf("c %d # \n", 1 - can_see_green_carpet);
    */
	//mylogfd(1, "be returning..");
    fflush(stdout);
    //mylogfd(1, "returning..");
    return 0;
}
