#ifndef NET_H
#define NET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

//modified by Jingyi
enum NET_COMMAND {
    N_NULL = 0,
    N_STOP,
    N_UPDATE,
    N_BLIND,
    N_TEST,
    N_PASSED,
    N_LINE,
    N_ANGLE
};
//end

struct net {
    int sock;
    struct sockaddr_in addr;
};

static inline void netc_init(struct net *p, char *ip, int port)
{
    if (port <= 0) {
        fprintf(stderr, "port should be positive\n");
        exit(-1);
    }

    if ((p->sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(-1);
    }

    memset(&p->addr, 0, sizeof(p->addr));
    p->addr.sin_family = PF_INET;
    p->addr.sin_port = htons(port);
    if (!inet_aton(ip, &p->addr.sin_addr)) {
        fprintf(stderr, "invalid ip: %s\n", ip);
        exit(-1);
    }
}

static inline void nets_init(struct net *p, int port)
{
    if (port <= 0) {
        fprintf(stderr, "port should be positive\n");
        exit(-1);
    }

    if ((p->sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(-1);
    }

    memset(&p->addr, 0, sizeof(p->addr));
    p->addr.sin_family = PF_INET;
    p->addr.sin_port = htons(port);

    if (bind(p->sock, (struct sockaddr *)&p->addr, sizeof(p->addr)) < 0) {
        perror("bind");
        exit(-1);
    }
}

static inline int net_send(void *buf, int buf_size, struct net *pc)
{
    return sendto(pc->sock, buf, buf_size, 0, (struct sockaddr *)&pc->addr, sizeof(struct sockaddr));
}


static inline int net_recv(void *buf, int buf_size, struct net *p)
{
    static socklen_t len = sizeof(struct sockaddr);
    return recvfrom(p->sock, buf, buf_size, 0, (struct sockaddr *)&p->addr, &len);
}

static inline void net_close(struct net *p)
{
    close(p->sock);
}

#endif

