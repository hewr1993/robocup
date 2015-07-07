#ifndef JPEG2YUV_H
#define JPEG2YUV_H

int clamp (double x);

void rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* Y1, unsigned char* Cb, unsigned char* Cr);

int jpeg2yuv(unsigned char* tmpbuf, int buffersize, unsigned char* yuvbuf, int* size);

#endif
