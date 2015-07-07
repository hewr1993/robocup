/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/select.h>
#include "sched.h"
#define NETFD -6

Info info;
Conf conf;
video_parse_info video_tmp[2];

static int videofd;
extern int logfd;
int flag = FLAG_ALL & ~ FLAG_SCHED;
int clientserver = 0;// 1为client，2为server
static const char *version = "robocuop-2.1";
//static int speed_parameter_init();
static const char *ipconf = "ip.conf";
static const char *logfile = "robot.log";
//static char* addr = "192.168.8.105";
static char* addr = "127.0.0.1";
/*
static char robot0[16] = "192.168.8.17";
static char robot1[16] = "192.168.8.19";
*/

static char robot0[16] = "127.0.0.1";
static char robot1[16] = "127.0.0.1";

extern void sendSTOP();
extern void info_init(Info *info);
extern void motor_thread_create();
extern void motor_init();
extern int cmd_close_all    (int argc, char **argv);
extern int cmd_close_video  (int argc, char **argv);
extern int cmd_close_net    (int argc, char **argv);
extern int cmd_close_sched  (int argc, char **argv);
extern int cmd_open_video   (int argc, char **argv);
extern int cmd_open_net     (int argc, char **argv);
extern int cmd_open_sched   (int argc, char **argv);
extern int cmd_open_all     (int argc, char **argv);
extern int cmd_task         (int argc, char **argv);
extern int cmd_video        (int argc, char **argv);
extern int cmd_search_ball  (int argc, char **argv);
extern int cmd_position     (int argc, char **argv);
extern int cmd_shoot        (int argc, char **argv);
extern int cmd_stop         (int argc, char **argv);
extern int cmd_rota         (int argc, char **argv);
extern int cmd_goto         (int argc, char **argv);
extern int cmd_run          (int argc, char **argv);
extern int cmd_sendAA       (int argc, char **argv);
extern int cmd_motor        (int argc, char **argv);
extern int cmd_help         (int argc, char **argv);
extern int cmd_quit         (int argc, char **argv);
extern int cmd_init         (int argc, char **argv);
extern int cmd_net          (int argc, char **argv);
extern int cmd_state        (int argc, char **argv);
extern int cmd_sendmsg      (int argc, char **argv);
extern int cmd_test         (int argc, char **argv);
extern int cmd_game         (int argc, char **argv);
extern int cmd_game_door         (int argc, char **argv);
static struct cmd cmdtab[] =  {
    { "call",      cmd_close_all },
    { "cvid",      cmd_close_video },
    { "cnet",      cmd_close_net },
    { "csch",      cmd_close_sched },
    { "ovid",      cmd_open_video },
    { "onet",      cmd_open_net },
    { "osch",      cmd_open_sched },
    { "oall",      cmd_open_all },

    { "task",      cmd_task },//--
    { "sbal",      cmd_search_ball },
    { "posn",      cmd_position },
    { "shoot",     cmd_shoot },
    { "stop",      cmd_stop},
    { "rota",      cmd_rota },
    { "goto",      cmd_goto },
    { "run" ,      cmd_run },
    { "sendAA",    cmd_sendAA },
    { "sendmsg",   cmd_sendmsg },
    { "init",      cmd_init },
    { "vido",      cmd_video },
    { "moto",      cmd_motor },
    { "help",      cmd_help },
    { "quit",      cmd_quit },
    { "net" ,      cmd_net},
    { "state",     cmd_state},
    { "game",      cmd_game},
    { "door",      cmd_game_door},
    { "test",      cmd_test},
    { 0 },
};
static pthread_mutex_t sch_mutex;
void scheduler ()
{
    pthread_mutex_lock(&sch_mutex);
    if (flag &FLAG_SCHED)
        sched(&info);
    pthread_mutex_unlock(&sch_mutex);
}
void* do_client(Info *info)
{
    mylogfd (1, "[robot]/main do_client %s\n", info->destip);
    netc_init(&(info->dest), info->destip, PORT);
    return NULL;
};
void* do_server(Info *info)
{
    mylogfd (1, "[robot]main do_server %s\n", info->ip);
    struct net ns;
    ns.addr.sin_addr.s_addr=inet_addr(info->ip);
    nets_init(&ns, PORT);

    const int size = 16;
    char buf[size +1];
    while (1) {
        mylogfd (1, "recv.....188\n");
        net_recv(buf, size, &ns);
        info->c_flag |= NEW_MSG;
        int len = strlen (buf);
        strncpy (info->msg, buf, len);
        info->msg[len] = '\0';
        mylogfd (1, "recv.....187\n");
        mylogfd (logfd, "[net]>%s\n", info->msg);
        mylogfd (1, "[net]>[%s]\n", info->msg);
        scheduler();

        mylogfd (1, "recv.....189\n");
    }
    return NULL;
};


static void handle_command (char *line)
{
    char **commands;
    struct cmd *c;
    int len;
    //mylogfd (1, "[robot]handle_command %s", line);
    if (line == NULL || strlen (line) < 4) return ;
    len = strlen (line) -1;
    while (line[len] == ' ' || line[len] == '\n' || line[len] == '\r') --len;
    line[len +1] = '\0';

    commands = strsplit_by_space (line, &len);
    if (len < 1)
        return;

    c = getcmd (commands[0], cmdtab);
    if (c == NULL)
        printf ("Unknown command: \"%s\"\n",line);
    else
        c->handler (len-1, commands+1);
    free (commands);
    scheduler();
}
static int find_max_fd ()
{
    int maxfd  = videofd+1;
    if (maxfd < STDIN_FILENO)
        maxfd = STDIN_FILENO + 1;
    return maxfd;
}
static void main_exit (int signo)
{
    //do_stop (&info);
    sendSTOP();
    signal (signo, SIG_DFL);
    raise (signo);
}
static void main_loop()
{
    mylogfd (1, "robot/main/loop clientserver=%d, flag=%d\n", clientserver,flag);
    fd_set rfds;
    int maxfd;
    char buf[BUFSIZE];
    //mylogfd(1,"size %d\n",BUFSIZE);
    FILE *vf = fdopen(videofd, "r");

    char *line;
    if (vf == NULL) {
        mylogfd(2, "open video file err");
        exit(-1);
    }
    signal (SIGINT,main_exit);

    maxfd = find_max_fd();
    mylogfd (1, "%d>>>", maxfd);
    //mylogfd (1, "\n >>>test");
    while (1) {
        //printf("in decision main\n");
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(videofd, &rfds);
        //FD_SET(network_fd, &readset);
    //mylogfd (1, "\n >>>test1");
        memset (buf, 0, sizeof (buf));
//mylogfd (1, "\n >>>test3");
        int nfds = select(maxfd+1, &rfds, NULL, NULL, NULL);
//mylogfd (1, "\n >>>test2");
        if (nfds == -1)
            err_sys("select");

        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            line = fgets (buf, BUFSIZE, stdin);
            mylogfd(logfd, "stdin>%s\n", line);
            handle_command (buf);
            mylogfd (1, ">>>");

        }


        if (FD_ISSET(videofd, &rfds)) {
 
            line = fgets(buf, BUFSIZE, vf);
       
            //mylogfd (logfd, "video>%s\n", line);
            if (flag & FLAG_VIDEO){
               
                handle_command(buf);
            }
        }
    }
}
static int ip_init(const char *conf)
{
    FILE* fp = fopen(conf, "r");
    if (!fp) {
        return -1;
    }
    if (fgets(robot0, 16, fp) != NULL)
        robot0[strlen(robot0)-1] = '\0';
    if (fgets(robot1, 16, fp) != NULL)
        robot1[strlen(robot1)-1] = '\0';
    fclose(fp);
    return 0;
}

static char usage[] = \
                      "[-a ip_address] [-g agcstr] [-h] [-f frame_rate] [-w waitshoottime]\n"\
                      "\t-a the ip address to send the videos\n"\
                      "\t-g agcstr\n"\
                      "\t-h help ,show this message\n"\
                      "\t-f frame_rate\n"\
                      "\t-w waitshoottime\n"
                      "\t c client\t s server\n";
static void show_usage()
{
    printf("Usage: %s\n%s\n", version, usage);
}

static int init(int num)
{
    //if (MOTOR_ON)
    ip_init(ipconf);
    info_init(&info);
    mylog_init(&logfd, logfile);    //初始化日志记录
    if (num != 0) {
        info.ip = robot0;
        info.destip  =robot1;
    } else {
        info.ip = robot1;
        info.destip = robot0;
    }
    mylogfd(1,"[robot]init INIT FINISHED ip=%s\n", info.ip);
    return 0;
}

int main(int argc, char **argv)
{
    char c = 's';
    double waitshoottime;
    pid_t video_pid;
    const char* agcstr = "10000";
    const char* frame_rate = "10";
    const char* waittime="2.0";
    int video_fd[2];
    // parse command line (todo)
    mylogfd(1, "[robot]main begain\n");
    printf("[robot]main begain....\n");
    int t = 0;
    while ((t = getopt(argc, argv, "a:g:hf:w:")) != -1) {
        switch (t) {
        case 'a':
            addr = strdup(optarg);
            break;
        case 'h':
            show_usage ();
            exit(0);
        case 'f':
            frame_rate = strdup(optarg);
            break;
        case 'g':
            agcstr = strdup(optarg);
            break;
            main_loop();
        case 'w':
            waittime=strdup(optarg);
            waitshoottime=atof(waittime);
            break;
        default:
            fprintf(stderr, "option error: %d\n", t);
            show_usage();
            exit(-1);
        }
    }
    if (optind < argc)
        c = argv[optind][0];
    mylogfd(1, "[robot]VIDEO PROCESS CREATED begain\n");
    init (c - 'c');
    if (c == 'c')
        clientserver = 1;
    else if (c == 's')
        clientserver = 2;
    // begin video process
    if (pipe(video_fd) < 0)
        err_sys("pipe error");
    if ((video_pid = fork()) < 0)
    {
	printf("fork failed\n");
	
        err_sys("fork error");
	}
    else if (video_pid == 0) {

    //printf("Test: %d\n", video_pid);
        close(video_fd[0]);  // video_fd[1] for writing
        dup2(video_fd[1], STDOUT_FILENO);
        close(video_fd[1]);

        if ( execl("./videocap", "videocap", "-a",
                   addr, "-g", agcstr, "-f", frame_rate,
                   (char *)0) < 0 )


    mylogfd(1, "in child\n");
	//printf("child");
    }
    // parent process
    close(video_fd[1]);
    videofd = video_fd[0];
    mylogfd(1, "[robot]VIDEO PROCESS CREATED\n");
    // motor thread
    //if (MOTOR_ON)
    motor_thread_create();
    mylogfd(1, "[robot]MOTOR THREAD CREATED\n");

    //启动服务器端，暂时去掉
    pthread_t serverthd;//modified
    if (clientserver > 0)
        pthread_create(&serverthd,NULL,do_server,&info);


    //初始化客户端，暂时去掉
    //pthread_t clientthd;//modified
    if (clientserver > 0)
        do_client(&info);

    mylogfd(1, "in main loop\n");
    main_loop();
    free(addr);
    return 0;
}

