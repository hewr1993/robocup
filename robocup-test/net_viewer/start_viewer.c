#include "net.h"
#include "cmv.h"
int use_cmv[2] = {1,1};
struct cmv_data data[2];
struct net ns[2];

// 初始化图像处理的进程
// 主要是网络
// name: 未知
// @param
// @return
void start_viewer(int argc, char *argv[])
{

    int port1 = 8000, port2 = 8001;
    const char *cfilename1="../vision/colors0.txt";
    const char *cfilename2="../vision/colors1.txt";

    for (;;) {
        int t = getopt(argc, argv, "r:bp:h");
        if (t == -1) {
            break;
        }

        switch (t) {
        case 'p':
            if (sscanf(optarg, "%d,%d", &port1, &port2) != 2) {
                fprintf(stderr, "error optarg: %s\n", optarg);
                exit(-1);
            }
            break;
        case 'h':
            fprintf(stderr, "Usage: %s [option]\n"
                    "-c s\tset color file s\n"
                    "-p n,m\tset port n (default 8000,8001)\n"
                    "-h\tprint this help\n", argv[0]);
            exit(0);
        case '?':
            break;
        default:
            fprintf(stderr, "option error: %d\n", t);
            exit(-1);
        }
    }

    if (optind != argc) {
        fprintf(stderr, "command line error\n");
        exit(-1);
    }

    nets_init(&(ns[0]), port1);
    nets_init(&(ns[1]), port2);
    printf("using port %d %d\n", port1, port2);

    if (use_cmv[0]) {
        cmv_init(&(data[0]), cfilename1);
        printf("using color file %s\n", cfilename1);
    }
    if (use_cmv[1]) {
        cmv_init(&(data[1]), cfilename2);
        printf("using color file %s\n", cfilename2);
    }
}
