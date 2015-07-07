#include <qpainter.h>
#include <qimage.h>
#include <math.h>
#include <string>
#include <qsocketnotifier.h>
#include "window.h"
#include "global.h"
#include "save.h"
#include "cvt.h"
#include "net.h"
#include "dec.h"
#include "cmv.h"
#include <stdio.h>

extern "C" {
#include "jpegdec.h"
}
/*by pch*/
char filename1[41];

/*by pch end*/
extern int use_cmv[2];
extern struct cmv_data data[2];
extern struct net ns[2];
int GW=500;
bool is_color_paint=true;
int n = 0;
int i1 = 0, i2 = 0;
char filename[24];
unsigned char raw_buf1[MAX_MJPEG_SIZE];
unsigned char raw_buf2[MAX_MJPEG_SIZE];
unsigned char yuv_buf1[YUV_SIZE];
unsigned char yuv_buf2[YUV_SIZE];
unsigned char *raw_buf[2];
unsigned char *yuv_buf[2];
struct spot p1[2];
struct spot p2[2];

struct spot left[2][MAX_LINES];
struct spot right[2][MAX_LINES];
int line_count[2];

struct local_parse_info local_info[2];

struct spot t_door_right[2];
struct spot t_door_left[2];
int t_is_door_right[2];
int t_is_door_left[2];

struct spot s_door_right[2];
struct spot s_door_left[2];
int s_is_door_right[2];
int s_is_door_left[2];

int image_count=0;
int edge_distance, edge_angle;

int point_n=0;
struct spot point_n_t[30];

char * stateParse(int i)
{
    char* result;
    switch (i) {
    case 2000:
        result="LOC_LEFT";
        break;
    case -2000:
        result="LOC_RIGHT";
        break;
    case -10000:
        result="LOC_INVALID";
        break;
    case 10000:
        result="LOC_CATCH_BALL";
        break;
    case 300:
        result="LOC_Y_FAR";
        break;
    case 9999:
        result="LOC_Y_INVALID";
        break;
    case 1000:
        result="LOC_Y_NEAR";
        break;
    case 500:
        result="LOC_Y_VERY_NEAR";
        break;
    case 0:
        result="Center";
        break;
    default:
        result="UNKNOW";
        break;
    }
    return result;
}
//
// name: 将x限制在[0,255]
// @param: x：输入的值
// @return
int clamp (double x)
{
    int r = x;      /* round to nearest */
    if (r < 0)
        return 0;
    else if (r > 255)
        return 255;
    return r;
}

//
// name: 由r，g，b计算y，u，v(YUV420P)
// @param
// @return
void rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* Y1, unsigned char* Cb, unsigned char* Cr)
{
    *Y1 = clamp((( ( 66 * r + 129 * g + 25 * b + 128) >> 8) + 16));
    *Cb = clamp(( ( -38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
    *Cr = clamp(( ( 112 * r - 94 * g - 18 * b + 128) >> 8) + 128 );
}

//
// name: 将网络传输过来的jpg数据，转成yuv格式的图片数据
// 一般而言处理的结果是有W*H个Y，有1/4*W*H个U及V分量
// @param
//		 	tmbuf：网络传输的数据
//		  	buffersize：数据的大小
//			yuvbuf：处理后的存储位置
//			size：存储后的大小
// @return  -1:失败;0:成功
int jpeg2yuv (unsigned char* tmpbuf, int buffersize, unsigned char* yuvbuf, int* size)
{
    struct jpeg_decompress_struct* jpeg_decompressor = newDecompressor ( MAX_NET_WIDTH );
    //每个像素有三个值，YUV
    long rgbbuffersize = MAX_NET_WIDTH * MAX_NET_HEIGHT * 3;
    unsigned char rgbbuffer[rgbbuffersize];

    //read the rgb info from the buffer
    if (read_JPEG_buffer(jpeg_decompressor, tmpbuf, buffersize, rgbbuffer, rgbbuffersize, NULL, 0) != 1) {
        fprintf(stderr, "\nerror while decoding jpeg files.\n");
        //if fatal error occurs,
        if (isfatalerror()) {
            fprintf(stderr, "\nwarning: fatal error occur. reconstructing decompress process.\n");
            //first delete old decompressor
            deleteDecompressor(jpeg_decompressor);
            //next create new decompressor
            jpeg_decompressor = newDecompressor(MAX_NET_WIDTH);
        }
        return -1;
    }
    unsigned char* yp = yuvbuf;			//y分量的首位置
    unsigned char* up = yp + W * H;		//u分量的首位置，y+点数
    unsigned char* vp = up + W * H / 4;	//v分量的首位置，u+点数/4
    unsigned char* rgbstart = rgbbuffer;
    int h, w;
    //now convert from rgb to YUV420P
    for (h = 0; h < H; h++) {
        for (w = 0; w < W; w++) {
            //r, g, b=>y, u, v
            rgb2yuv( *(rgbstart+0), *(rgbstart+1), *(rgbstart+2), yp, up, vp);
            yp++;
            if ((h % 2 == 1) && (w % 2 == 1)) {
                up++;
                vp++;
            }
            rgbstart += 3;
        }
    }
    *size = W * H * 3 / 2;
    return 0;
}

window::window()
{
    setMinimumSize(W+GW, H*2);
    setMaximumSize(W+GW, H*2);
    setBackgroundMode(Qt::NoBackground);

    pRGB[0] = new QImage(W, H, 32, 0);
    pRGB[1] = new QImage(W, H, 32, 0);
    dec_init();
    translate_init(local_info);

    raw_buf[0]=raw_buf1;
    raw_buf[1]=raw_buf2;
    yuv_buf[0]=yuv_buf1;
    yuv_buf[1]=yuv_buf2;
    QSocketNotifier *sn1 = new QSocketNotifier(ns[0].sock, QSocketNotifier::Read, this);
    QSocketNotifier *sn2 = new QSocketNotifier(ns[1].sock, QSocketNotifier::Read, this);
    //将处理函数与信号链接起来
    connect(sn1, SIGNAL(activated(int)), this, SLOT(get_data1()));
    connect(sn2, SIGNAL(activated(int)), this, SLOT(get_data2()));
}

window::~window()
{
    printf("I'm over'\n");
}

void window::paintEvent(QPaintEvent *)
{
    int pensize=3;

    int placeSize=GW/2;
    QPainter p(this);
    p.drawImage(0, 0, *pRGB[0]);
    p.drawImage(0, H, *pRGB[1]);
    p.setPen(QPen(QColor(255, 255, 255),pensize));
    if (p2[0].x > 0)
        p.drawRect(p1[0].x << 1, p1[0].y << 1, (p2[0].x - p1[0].x) << 1,(p2[0].y - p1[0].y + 1) << 1 );
    if (t_door_right[0].x > 0) {
        p.drawLine(t_door_left[0].x, t_door_left[0].y, t_door_right[0].x, t_door_right[0].y);
        p.setPen(QPen(QColor(0, 0, 0),pensize));
        p.drawEllipse(t_door_left[0].x-5, t_door_left[0].y-5, 10, 10);
        p.drawEllipse(t_door_right[0].x-5, t_door_right[0].y-5, 10, 10);
    }
    p.setPen(QPen(QColor(255, 255, 255),pensize));
    if (p2[1].x > 0)
        p.drawRect((p1[1].x << 1), H+(p1[1].y << 1), (p2[1].x - p1[1].x) << 1, (p2[1].y - p1[1].y + 1) << 1 );
    if (t_door_right[1].x > 0) {
        p.drawLine(t_door_left[1].x, H+t_door_left[1].y, t_door_right[1].x, H+t_door_right[1].y);
        p.setPen(QPen(QColor(0, 0, 0),pensize));
        p.drawEllipse(t_door_left[1].x-5, H+t_door_left[1].y-5, 10, 10);
        p.drawEllipse(t_door_right[1].x-5, H+t_door_right[1].y-5, 10, 10);
    }

    if (s_door_right[0].x > 0) {
        p.drawLine(s_door_left[0].x, s_door_left[0].y, s_door_right[0].x, s_door_right[0].y);
        p.setPen(QPen(QColor(0, 0, 0),pensize));
        p.drawEllipse(s_door_left[0].x-5, s_door_left[0].y-5, 10, 10);
        p.drawEllipse(s_door_right[0].x-5, s_door_right[0].y-5, 10, 10);
    }
    p.setPen(QPen(QColor(255, 255, 255),pensize));
    if (p2[1].x > 0)
        p.drawRect((p1[1].x << 1), H+(p1[1].y << 1), (p2[1].x - p1[1].x) << 1, (p2[1].y - p1[1].y + 1) << 1 );
    if (s_door_right[1].x > 0) {
        p.drawLine(s_door_left[1].x, H+s_door_left[1].y, s_door_right[1].x, H+s_door_right[1].y);
        p.setPen(QPen(QColor(0, 0, 0),pensize));
        p.drawEllipse(s_door_left[1].x-5, H+s_door_left[1].y-5, 10, 10);
        p.drawEllipse(s_door_right[1].x-5, H+s_door_right[1].y-5, 10, 10);
    }

    //draw ball
    p.setPen(QPen(QColor(255, 255, 255),pensize));
    if (local_info[0].ball_radius != -1) {
        //p.drawEllipse(local_info[0].ball_center.x-5, local_info[0].ball_center.y-5, 10, 10);
        p.drawEllipse(local_info[0].ball_center.x-local_info[0].ball_radius, local_info[0].ball_center.y-local_info[0].ball_radius, 2*local_info[0].ball_radius, 2*local_info[0].ball_radius);
    }
    if (local_info[1].ball_radius != -1) {
        //p.drawEllipse(local_info[1].ball_center.x-5, H+local_info[1].ball_center.y-5, 10, 10);
        p.drawEllipse(local_info[1].ball_center.x-local_info[1].ball_radius, H+local_info[1].ball_center.y-local_info[1].ball_radius, 2*local_info[1].ball_radius, 2*local_info[1].ball_radius);
    }
    p.setPen(QPen(QColor(255, 0, 255),pensize));
    for (int i=0; i<line_count[0]; i++)
        p.drawLine(left[0][i].x,left[0][i].y,right[0][i].x,right[0][i].y);
    //p.setPen(QPen(QColor(160, 32, 240),pensize));
    for (int i=0; i<line_count[1]; i++)
        p.drawLine(left[1][i].x,left[1][i].y+H,right[1][i].x,right[1][i].y+H);


    //////////////////////////paint ground

    p.setPen(Qt::black);
    p.setBrush(Qt::black);
    p.drawRect(W, 0, GW,2*H);

    double x_fac=3,y_fac=1.5;
    p.setPen(Qt::white);
    p.setBrush(Qt::white);
    p.drawRect( W+placeSize-20, 2*H-20, 40, 20);
    int i=0;
    for (i=0; i<2; i++) {
        if (i==0) {
            p.setPen(QPen(Qt::yellow,pensize));
            p.setBrush(Qt::yellow);
        } else {
            p.setPen(QPen(Qt::green,pensize));
            p.setBrush(Qt::green);
        }
        if (local_info[i].is_ball_seen) {
            fprintf(stderr,"The ball %d is: %d, %d, %s, %s\n",i,local_info[i].ball_acc_pos.x, local_info[i].ball_acc_pos.y,stateParse(local_info[i].ball_pos.x),stateParse(local_info[i].ball_pos.y));
            p.drawEllipse( W+placeSize + x_fac*local_info[i].ball_acc_pos.x-5, 2*H-y_fac*local_info[i].ball_acc_pos.y-5, 10, 10);
        }
        if (i==0) {
            p.setPen(QPen(QColor(194,93,220),pensize));
            p.setBrush(QColor(194,93,220));
        } else {
            p.setPen(QPen(QColor(250,184,51),pensize));
            p.setBrush(QColor(250,184,51));
        }
        if (local_info[i].enemy_door.is_door_seen) {
            fprintf(stderr,"The door %d is: %d, %d, %s, %s, %d, %d\n",i,local_info[i].enemy_door.is_door_left, local_info[i].enemy_door.is_door_right, stateParse(local_info[i].enemy_door.door_pos.x),stateParse(local_info[i].enemy_door.door_pos.y), local_info[i].enemy_door.door_acc_pos.x,local_info[i].enemy_door.door_acc_pos.y);
            //fprintf(stderr,"The door %d is: %d, %d, %d, %d, %d, %d, %d, %d\n",i,local_info[i].enemy_door.is_door_left, local_info[i].enemy_door.is_door_right, local_info[i].enemy_door.door_left.x,local_info[i].enemy_door.door_left.y, local_info[i].enemy_door.door_right.x, local_info[i].enemy_door.door_right.y,local_info[i].enemy_door.door_pos.x,local_info[i].enemy_door.door_pos.y);
            p.drawEllipse(W+placeSize+x_fac*local_info[i].enemy_door.door_pos.x-5, 2*H-y_fac*local_info[i].enemy_door.door_pos.y-5, 10,10);
            if (local_info[i].enemy_door.is_door_left)
                p.drawRect(W+placeSize + x_fac*local_info[i].enemy_door.door_left.x-5, 2*H-y_fac*local_info[i].enemy_door.door_left.y-5, 10, 10);
            if (local_info[i].enemy_door.is_door_right)
                p.drawRect(W+placeSize + x_fac*local_info[i].enemy_door.door_right.x-5, 2*H-y_fac*local_info[i].enemy_door.door_right.y-5, 10, 10);
            if (local_info[i].enemy_door.is_door_left&&local_info[i].enemy_door.is_door_right)
                p.drawLine(W+placeSize + x_fac*local_info[i].enemy_door.door_left.x, 2*H-y_fac*local_info[i].enemy_door.door_left.y, W+placeSize + x_fac* local_info[i].enemy_door.door_right.x, 2*H-y_fac*local_info[i].enemy_door.door_right.y);
        }

        if (local_info[i].mine_door.is_door_seen) {
            fprintf(stderr,"The mine door %d is: %d, %d, %s, %s, %d, %d\n",i,local_info[i].mine_door.is_door_left, local_info[i].mine_door.is_door_right, stateParse(local_info[i].mine_door.door_pos.x),stateParse(local_info[i].mine_door.door_pos.y), local_info[i].mine_door.door_acc_pos.x,local_info[i].mine_door.door_acc_pos.y);
            //fprintf(stderr,"The door %d is: %d, %d, %d, %d, %d, %d, %d, %d\n",i,local_info[i].mine_door.is_door_left, local_info[i].mine_door.is_door_right, local_info[i].mine_door.door_left.x,local_info[i].mine_door.door_left.y, local_info[i].mine_door.door_right.x, local_info[i].mine_door.door_right.y,local_info[i].mine_door.door_pos.x,local_info[i].mine_door.door_pos.y);
            p.drawEllipse(W+placeSize+x_fac*local_info[i].mine_door.door_pos.x-5, 2*H-y_fac*local_info[i].mine_door.door_pos.y-5, 10,10);
            if (local_info[i].mine_door.is_door_left)
                p.drawRect(W+placeSize + x_fac*local_info[i].mine_door.door_left.x-5, 2*H-y_fac*local_info[i].mine_door.door_left.y-5, 10, 10);
            if (local_info[i].mine_door.is_door_right)
                p.drawRect(W+placeSize + x_fac*local_info[i].mine_door.door_right.x-5, 2*H-y_fac*local_info[i].mine_door.door_right.y-5, 10, 10);
            if (local_info[i].mine_door.is_door_left&&local_info[i].mine_door.is_door_right)
                p.drawLine(W+placeSize + x_fac*local_info[i].mine_door.door_left.x, 2*H-y_fac*local_info[i].mine_door.door_left.y, W+placeSize + x_fac* local_info[i].mine_door.door_right.x, 2*H-y_fac*local_info[i].mine_door.door_right.y);
        }
    }

    if (local_info[0].is_ball_seen&&local_info[1].is_ball_seen) {
        p.setPen(Qt::red);
        p.setBrush(Qt::red);
        int t_x=(local_info[1].ball_acc_pos.x+local_info[0].ball_acc_pos.x)/2;
        int t_y=(local_info[1].ball_acc_pos.y+local_info[0].ball_acc_pos.y)/2;
        fprintf(stderr,"The ball is: %d, %d\n",t_x,t_y);
        p.drawEllipse( W+placeSize + x_fac*t_x-10, 2*H-y_fac*t_y-10, 20, 20);
    }

    p.setPen(QPen(QColor(255, 255, 255),pensize));
    for (int i=0; i<line_count[0]; i++) {
        int tmp_l_x,tmp_l_y,tmp_r_x,tmp_r_y;
        getxy(left[0][i].x,left[0][i].y,&tmp_l_x,&tmp_l_y,local_info[0].vparam);
        getxy(right[0][i].x,right[0][i].y,&tmp_r_x,&tmp_r_y,local_info[0].vparam);
        if (tmp_l_x<-placeSize) {
            if (tmp_l_y!=tmp_r_y) {
                tmp_l_y=(-placeSize-tmp_l_x)*(tmp_r_x-tmp_l_x)/(tmp_r_y-tmp_l_y)+tmp_l_y;
            }
            tmp_l_x=-placeSize;
        }
        p.drawLine(W+placeSize + x_fac*tmp_l_x,2*H-y_fac*tmp_l_y,W+placeSize + x_fac*tmp_r_x,2*H-y_fac*tmp_r_y);
    }
    p.setPen(QPen(QColor(200, 200, 200),pensize));
    for (int i=0; i<line_count[1]; i++) {
        int tmp_l_x,tmp_l_y,tmp_r_x,tmp_r_y;
        getxy(left[1][i].x,left[1][i].y,&tmp_l_x,&tmp_l_y,local_info[1].vparam);
        getxy(right[1][i].x,right[1][i].y,&tmp_r_x,&tmp_r_y,local_info[1].vparam);
        if (tmp_l_x<-placeSize) {
            if (tmp_l_y!=tmp_r_y) {
                tmp_l_y=(-placeSize-tmp_l_x)*(tmp_r_x-tmp_l_x)/(tmp_r_y-tmp_l_y)+tmp_l_y;
            }
            tmp_l_x=-placeSize;
        }
        p.drawLine(W+placeSize + x_fac*tmp_l_x,2*H-y_fac*tmp_l_y,W+placeSize + x_fac*tmp_r_x,2*H-y_fac*tmp_r_y);
    }
    if (point_n>0) {
        p.setPen(Qt::red);
        for (int i=0; i<point_n; i++) {
            p.drawEllipse(point_n_t[i].x-3,point_n_t[i].y-3,3,3);
        }
    }
}
void get_info(int num,QImage *pRGBT)
{
    //fprintf(stderr, "get data 1\n");
    int size = 0, outsize = 0;
    //从socket(ns1)将数据写入到raw_buf中
    if ((size = net_recv(raw_buf[num], MAX_MJPEG_SIZE, &(ns[num]))) <= 0) {
        fprintf(stderr,"net_recv error\n");
        return;
    }

    unsigned char tmp[MAX_NET_WIDTH*MAX_NET_HEIGHT*2];
    //将raw_buf1的数据，转换成YUV420P写入到tmp所在内存
    jpeg2yuv(raw_buf[num], size, tmp, &outsize);
    //拷贝内存信息
    memcpy(yuv_buf[num], tmp, outsize);
    //fprintf(stderr, "%d , %d\n",outsize, YUV_SIZE);

    if (use_cmv[num]) {

        cmv_process(&(data[num]), yuv_buf[num]);

        struct local_parse_info * local_tmp=&(local_info[num]);
        local_tmp->is_ball_acc = 0;
        local_tmp->is_ball_seen=0;

        local_tmp->enemy_door.is_door_seen=0;
        local_tmp->enemy_door.is_door_left=0;
        local_tmp->enemy_door.is_door_right=0;

        local_tmp->mine_door.is_door_seen=0;
        local_tmp->mine_door.is_door_left=0;
        local_tmp->mine_door.is_door_right=0;

        local_tmp->is_robot_seen=0;
        local_tmp->is_robot_acc=0;

        struct reg_t *reg = data[num].cmax[COLOR_B];
        if (reg != 0) {
            p1[num].x = reg->x1;
            p2[num].x = reg->x2;
            p1[num].y = reg->y1;
            p2[num].y = reg->y2;

            find_ball(num, &(data[num]),local_tmp);
            //fprintf(stderr,"The ball 1 is: %d, %d, %d\n",local_info[0].is_ball_seen,local_info[0].ball_pos.x,local_info[0].ball_pos.y);
        } else
            p2[num].x = 0;
        if (!ball_outline_detection(&(data[num]), &(local_tmp->ball_center.x), &(local_tmp->ball_center.y), &local_tmp->ball_radius))
            local_tmp->ball_radius = -1;

        if (data[num].cmax[COLOR_G] != 0) {
            goal_detection(&(data[num]), yuv_buf[num], &(t_door_left[num]), &(t_door_right[num]), &(t_is_door_right[num]), &(t_is_door_left[num]),local_tmp,&local_tmp,COLOR_G);
            find_door(num,&(data[num]),local_tmp,yuv_buf[num],0);
        } else
            t_door_right[num].x = 0;

        if (data[num].cmax[COLOR_S] != 0) {
            goal_detection(&(data[num]), yuv_buf[num], &(s_door_left[num]), &(s_door_right[num]), &(s_is_door_right[num]), &(s_is_door_left[num]),local_tmp,&local_tmp,COLOR_S);
            find_door(num,&(data[num]),local_tmp,yuv_buf[num],1);
        } else
            s_door_right[num].x = 0;

        //障碍物
        if (data[num].cmax[COLOR_R]!=0) {
            find_robot(num, &(data[num]),local_tmp);
            //fprintf(stderr,"ROBO: %d, %d, %d, %d, %d, %d\n",local_tmp->is_robot_seen,local_tmp->is_robot_acc,local_tmp->robot_pos.x,local_tmp->robot_pos.y,local_tmp->robot_acc_pos.x,local_tmp->robot_acc_pos.y);
        }

        double angle[MAX_LINES];
        int radius[MAX_LINES];
        edge_angle = 180;
        edge_distance = 200;
        double far = 1000;
        double tmp_ang;
        int tmp_dis;
        judge_outside(&(data[num]),local_tmp);
        //fprintf(stderr,"                                                             \r");
        //fprintf(stderr,"OUT %d: %d, %d\n",num,local_tmp->is_outside,local_tmp->green_sum);
        edge_detection(&(data[num]), angle, radius, &(line_count[num]));
        for (int i=0; i<line_count[num]; i++) {
            //printf("%f %d\n", angle[i], radius[i]);
            if (abs(angle[i])<3) {
                left[num][i].x = 0;
                left[num][i].y = 2*(H_2 - 1 - (int)(radius[i] / sin(angle[i])));
                right[num][i].x = 2*(W_2 - 1);
                right[num][i].y = 2*(H_2 - 1 - (int)((radius[i] - (W_2 - 1)*cos(angle[i])) / sin(angle[i])));
            } else {
                left[num][i].x = 2*(int)(radius[i] / cos(angle[i]));
                left[num][i].y = 2*(H_2 - 1 );
                right[num][i].x = 2*(int)((radius[i] - (H_2 - 1)*sin(angle[i])) / cos(angle[i]));
                right[num][i].y = 2;
            }
            getEdgeDistance(left[num][i], right[num][i], &tmp_dis, &tmp_ang,local_tmp);
            double tmp_far = fabs(tmp_dis) * fabs(cos(tmp_ang));
            int tmp_angle = (int)(tmp_ang * 180 / PI);
            if (((abs(tmp_angle - edge_angle) < 5 && tmp_far < far)
                    || abs(tmp_angle) < abs(edge_angle) -5)&&tmp_dis>=0&&tmp_dis<=500) {
                edge_distance = (int)tmp_dis;
                edge_angle = tmp_angle;
                far = tmp_far;
            }
        }

        //fprintf(stderr,"Edge %d: %d, %d\n",num,edge_distance,edge_angle);
        //int distance;
        //double angle;
        //getEdgeDistance(e_l,e_r, &distance, &angle,&(local_info[0]));
        //}
        if (is_color_paint)
            cvtc(yuv_buf[num], pRGBT->bits(), data[num].map, data[num].rgb);
        else
            cvt(yuv_buf[num], pRGBT->bits());
    } else {
        cvt(yuv_buf[num], pRGBT->bits());
    }
    //image_count++;
    //fprintf(stderr, "\r image_count%d", image_count);
}
void window::get_data1()
{
    get_info(0,pRGB[0]);
    update();

}

void window::get_data2()
{
    get_info(1,pRGB[1]);
    update();
}

void window::mousePressEvent(QMouseEvent *e)
{
    is_color_paint=0;
    if (e->button() != RightButton && e->button() != LeftButton) {
        int x=e->x(),y=e->y();
        int x_t,y_t;
        if (y<=H) {
            getxy(x,y,&x_t,&y_t,local_info[0].vparam);
            fprintf(stderr,"&& %d %d %d %d\n",x,y,x_t,y_t);
        } else {
            getxy(x,y-H,&x_t,&y_t,local_info[1].vparam);
            fprintf(stderr,"&& %d %d %d %d\n",x,y-H,x_t,y_t);
        }
        sprintf(filename, "%d %d",x_t, y_t);
        setCaption(filename);
	//is_color_paint=!is_color_paint;
        return;
    } else if(e->button() == RightButton){

        /*int x=e->x(),y=e->y();
        int x_t,y_t;
        if (y<=H) {
            getxy(x,y,&x_t,&y_t,local_info[0].vparam);
            point_n_t[point_n].x=x;
            point_n_t[point_n].y=y;
        } else {
            getxy(x,y-H,&x_t,&y_t,local_info[1].vparam);
            point_n_t[point_n].x=x;
            point_n_t[point_n].y=y;
        }
        ++point_n;*/
	point_n_t[0].x = 0;
	point_n_t[0].y = 50;
	point_n_t[1].x = 10;
	point_n_t[1].y = 50;
	point_n_t[2].x = -10;
	point_n_t[2].y = 50;	
	point_n_t[3].x = 0;
	point_n_t[3].y = 60;
	point_n_t[4].x = 10;
	point_n_t[4].y = 60;
	point_n_t[5].x = -10;
	point_n_t[5].y = 60;
	
	return;
    }
    else{
	int x=e->x(),y=e->y();
        int x_t,y_t;
        if (y<=H) {
            getxy(x,y,&x_t,&y_t,local_info[0].vparam);
        } else {
            getxy(x,y-H,&x_t,&y_t,local_info[1].vparam);
        }
 
    	++n;
    	sprintf(filename, "%d_1.yuv", n);
    	msave(filename, yuv_buf[0], MAX_MJPEG_SIZE);
    	sprintf(filename, "%d_2.yuv", n);
    	msave(filename, yuv_buf[1], MAX_MJPEG_SIZE);
    	sprintf(filename, "(%d %d)%d.yuv", x_t, y_t, n
);
	
        
	int i = 0 ;
        std::string filename_tmp = "../new_file_show/1";
	for(i = 0 ; i < 17 ; i++)
        {
	  filename1[i] =filename_tmp[i];
        }
	 
	//for(i = 17 ; i < 41 ; i++)
	 // filename1[i] = filename[i-17];
	sprintf(filename, "%d_1.yuv", n);
	for(i = 17 ; i < 41 ; i++)
	  filename1[i] = filename[i-17];
	
    	msave(filename1, yuv_buf[0], MAX_MJPEG_SIZE);
    	sprintf(filename, "%d_2.yuv", n);
	for(i = 17 ; i < 41 ; i++)
	  filename1[i] = filename[i-17];
	
    	msave(filename1, yuv_buf[1], MAX_MJPEG_SIZE);
	
    	setCaption(filename);
	}
}
