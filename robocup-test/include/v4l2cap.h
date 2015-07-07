/**
 * v4l2cap.h
 *
 * 定义了基于v4l2标准的视频采集接口，以及相关的摄像头视频采集
 * 的数据结构，同时更主要是一些相关的常量的说明和定义
 *
 * @时间 2007/5/20
 * @作者 jascon
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/mman.h>

#include <linux/videodev.h>

//added header files
#include "utils.h"

#define NB_BUFFERS	4
#define IO_METHOD	V4L2_IO_METHOD_MMAP

//视频数据结构，保存了所有的采集到么一个祯的视频信息
struct vd_capture {
    //设备信息
    int	fd;	//设备对应文件描述符
    char	*dev_name; //设备名称

    //图像采集相应参数
    unsigned int	fps;
    unsigned int 	width;
    unsigned int	height;
    unsigned int    framesize;
    int	format;

    //v4l2 对应的数据结构
    struct v4l2_capability	cap;
    struct v4l2_buffer	buf;
    struct v4l2_format	fmt;
    struct v4l2_requestbuffers	rbuf;

    //存放数据的变量
    void * mem[NB_BUFFERS];
//	unsigned char	*decbuffer;
    unsigned char	*framebuffer;
    unsigned char 	*rgbbuffer;
    unsigned char   *yuvbuffer;
//	unsigned char 	*tmpbuffer;

    //控制变量，用于是否退出采集
    int	quit;
    int 	streaming;
};

//函数声明体
int init_data(struct vd_capture * vd_cap, char *dev_name,
              unsigned int fps,
              unsigned int width,
              unsigned int height,
              int format);
int init_device(struct vd_capture * vd_cap);
int capture(struct vd_capture * vd_cap);
int close_device(struct vd_capture * vd_cap);
int enable_device(struct vd_capture * vd_cap);
int disable_device(struct vd_capture * vd_cap);
