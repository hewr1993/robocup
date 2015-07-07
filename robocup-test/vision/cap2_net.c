#include <sys/poll.h>
#include <stdlib.h>
#include "global.h"
#include "process.h"
#include "jpegdec.h"
#include "net.h"

void init_video_work(struct vd_capture* c, int framerate, char * instrument_path)
{
    if (c == NULL) {
        mylogfd(2,"[Version] When get the memory for vd_cap\n");
        exit(1);
    }
    if (init_data(c, instrument_path, framerate, W, H, V4L2_PIX_FMT_MJPEG)==-1) {
        mylogfd(2,"[Version] When initial the data struct\n");
        exit(1);
    }
    //调用init_device()进行初始化视频硬件环境

    if (init_device(c) < 0) {
        mylogfd(2,"[Version] When initial the device %s\n",instrument_path);  //initial the video failed;
        exit(1);
    }

}
/*
 *
 * @vara
 * @return
 *
 * */
int main(int argc, char *argv[])
{

    int id1 = 0, id2 = 1;
    int framerate = 5;
    char ip[16] = "127.0.0.1";
    int port1 = 8000, port2 = 8001;
    int agc = -1;

    //struct cam c1, c2;
    struct vd_capture* c1;
    struct vd_capture* c2;
    struct net nc1, nc2;

    //open log
    mylog_init(&logfd,"log_version.txt");

    for (;;) {
        int t = getopt(argc, argv, "d:f:a:p:h:g:?");
        if (t == -1) {
            break;
        }
        switch (t) {
        case 'd':
            if (sscanf(optarg, "%d,%d", &id1, &id2) != 2) {
                mylogfd(2,"[Version] Error optarg: %s\n", optarg);
                exit(-1);
            }
            break;
        case 'f':
            if (sscanf(optarg, "%d", &t) != 1) {
                mylogfd(2,"[Version] Error optarg: %s\n", optarg);
                exit(-1);
            }
            if (t <= 0 || t > 30) {
                mylogfd(2,"[Version] Framerate should be (0, 30]: not %d\n",t);
                exit(-1);
            }
            framerate = t;
            break;
        case 'a':
            if (sscanf(optarg, "%s", ip) != 1) {
                mylogfd(2,"[Version] Error optarg: %s\n", optarg);
                exit(-1);
            }
            break;
        case 'p':
            if (sscanf(optarg, "%d,%d", &port1, &port2) != 2) {
                mylogfd(2,"[Version] Error optarg: %s\n", optarg);
                exit(-1);
            }
            break;
        case 'h':
            mylogfd(1,"[Version] Usage: %s [option]\n"
                    "-d n,m\tset cam id n (default 0,1)\n"
                    "-f n\tset framerate n (default 5)\n"
                    "-a s\tset addr s (default 127.0.0.1)\n"
                    "-p n,m\tset port n (default 8000,8001)\n"
                    "-h\tprint this help\n", argv[0]);
            return 0;
        case 'g':
            agc = atoi(optarg);
            break;
        case '?':
            break;
        default:
            mylogfd(2,"[Version] Option error: %d\n", t);
            exit(-1);
        }
    }
    if (optind != argc) {
        mylogfd(2,"[Version] Command line error\n");
        exit(-1);
    }

    //start the init work

    //init video instrument

    c1 = (struct vd_capture*) calloc(1, sizeof(struct vd_capture));
    //init_video_work(c1, framerate, "/dev/video1");
    init_video_work(c1, framerate, "/dev/video2");//just to make in my notebook

    c2 = (struct vd_capture*) calloc(1, sizeof(struct vd_capture));
    //init_video_work(c2, framerate, "/dev/video2");
    init_video_work(c2, framerate, "/dev/video1");
    mylogfd(1,"[Version] Using video%d video%d (framerate %d)\n",id1, id2, framerate);

    //init net work
    netc_init(&nc1, ip, port1);
    netc_init(&nc2, ip, port2);
    mylogfd(1,"[Version] Using ip %s port %d %d\n", ip, port1, port2);

    //init image deal
    process_init();

    //begin to capture
    int captured1=0, captured2=0;
    //int index=0;
    mylogfd(1,"[Version] Begin to capture\n");
    while (!c1->quit && !c2->quit) {
        //开始采集视频数据
        //waiting
        if ((captured1=capture(c1)) < 0) {
            mylogfd(2,"[Version/Cap/main] Error occurs when capturing video0\n");
            break;
        }
        if ((captured2=capture(c2)) < 0) {
            mylogfd(2,"[Version/Cap/main] Error occurs when capturing video1\n");
            break;
        }
        //posting
        /*if(index%10==0){
        	mylogfd(logfd,"\r%d size: %d",index,captured1);
        	mylogfd(logfd,"\r%d size: %d",index,captured2);
        }
        index++;*/
	//mylogfd(1,"[Version] send pic\n");
        if (net_send(c1->framebuffer, captured1, &nc1)==-1) {
            mylogfd(2,"[Version|Cap|main] The image file is too large: size %d.\n", captured1);
            continue;
        }

        process(c1, 0);
        if (net_send(c2->framebuffer, captured2, &nc2)==-1) {
            mylogfd(2,"[Version|Cap|main] The image file is too large: size %d.\n", captured2);
            continue;
        }
        process(c2, 1);
	//mylogfd(1,"[Version] send pic over\n");
        out_info();
        //susleep(1000000);
    }
    mylogfd(1,"[Version|Cap|main] End capture\n");
    return 0;
}


