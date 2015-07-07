#ifndef CAM_H
#define CAM_H

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/videodev.h>

#include "global.h"
#include "pwc-ioctl.h"

struct cam {
    int fd;
    struct video_mmap vm;
    unsigned char *src;
    unsigned char *vidbuf;
    struct video_mbuf vmbuf;
};

inline void cam_init(struct cam *pc, int id)
{

    if (id != 0 && id != 1) {
        fprintf(stderr, "video id should be 0 or 1\n");
        exit(-1);
    }

    char dev[12];
    sprintf(dev, "/dev/video%d", id);

    if ((pc->fd = open(dev, O_RDONLY)) < 0) {
        perror(dev);
        exit(-1);
    }

    struct video_picture picture;
    if (ioctl(pc->fd, VIDIOCGPICT, &picture) < 0) {
        perror("VIDIOCGPICT");
        exit(-1);
    }
    if (picture.palette != VIDEO_PALETTE_RAW) {
        picture.palette = VIDEO_PALETTE_RAW;
        if (ioctl(pc->fd, VIDIOCSPICT, &picture) < 0) {
            perror("VIDIOCSPICT");
            exit(-1);
        }
    }

    // compression
    int t = 3;
    if (ioctl(pc->fd, VIDIOCPWCSCQUAL, &t) < 0) {
        perror("VIDIOCPWCSCQUAL");
        exit(-1);
    }

    // noise;
    t = 1;	// should be 3, driver's bug?
    if (ioctl(pc->fd, VIDIOCPWCSDYNNOISE, &t) < 0) {
        perror("ioctl VIDIOCPWCSDYNNOISE");
        exit(-1);
    }

    if (ioctl(pc->fd, VIDIOCGMBUF, &pc->vmbuf) < 0) {
        perror("VIDIOCGMBUF");
        exit(-1);
    }
    if (pc->vmbuf.frames < 2) {
        fprintf(stderr, "double buffer fail\n");
        exit(-1);
    }

    pc->vidbuf = (unsigned char *)mmap(0, pc->vmbuf.size, PROT_READ, MAP_SHARED, pc->fd, 0);
    if (pc->vidbuf == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }

    struct video_window vwin;
    if (ioctl(pc->fd, VIDIOCGWIN, &vwin) < 0) {
        perror("VIDIOCGWIN");
        exit(-1);
    }
    t = R << PWC_FPS_SHIFT;
    if ((vwin.width - W) || (vwin.height - H) ||
            ((vwin.flags & PWC_FPS_FRMASK) - t)) {
        vwin.width = W;
        vwin.height = H;
        vwin.flags &= ~PWC_FPS_FRMASK;
        vwin.flags |= t;
        if (ioctl(pc->fd, VIDIOCSWIN, &vwin) < 0) {
            perror("VIDIOCSWIN");
            exit(-1);
        }
    }

    pc->vm.format = VIDEO_PALETTE_RAW;
    pc->vm.width = W;
    pc->vm.height = H;
    pc->vm.frame = 1;
}

inline void cam_capture(struct cam *pc)
{
    pc->vm.frame ^= 0x1;
    if (ioctl(pc->fd, VIDIOCMCAPTURE, &pc->vm) < 0) {
        perror("VIDIOCMCAPTURE");
        exit(-1);
    }
}

inline void cam_sync(struct cam *pc)
{
    if (ioctl(pc->fd, VIDIOCSYNC, &pc->vm.frame) < 0) {
        perror("VIDIOCSYNC");
        exit(-1);
    }
    pc->src = pc->vidbuf + pc->vmbuf.offsets[pc->vm.frame];
}

inline void cam_close(struct cam *pc)
{
    munmap(pc->vidbuf, pc->vmbuf.size);
    close(pc->fd);
}

#endif

