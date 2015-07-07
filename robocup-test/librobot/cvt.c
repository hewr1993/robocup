#include "cvt.h"
#include <stdio.h>

static inline unsigned char sat(int c)
{
return( (c & ~255)? (c<0)?0:255 : c );
}

void cvt(const unsigned char * restrict py1, unsigned char * restrict pd1)
{
    const unsigned char *py2 = py1 + W;
    const unsigned char *pu = py1 + Y_SIZE;
    const unsigned char *pv = pu + U_SIZE;
    unsigned char *pd2 = pd1 + W*4;

    int y, u, v;
    int b, g, r;
    int i, j;
    for (i=0; i<H_2; ++i) {
        for (j=0; j<W_2; ++j) {


            u = *(pu++) - 128;
            v = *(pv++) - 128;

            b = (454*u) >> 8;
            g = (183*v + 88*u) >> 8;
            r = (359*v) >> 8;

            y = *(py1++);

            //if (i == 0 && j == 0) {
            //	fprintf(stderr, "(%d %d %d)\n", y, u+128, v+128);
            //}

            *(pd1++) = sat(y + b);
            *(pd1++) = sat(y - g);
            *(pd1++) = sat(y + r);
            ++pd1;

            y = *(py1++);
            *(pd1++) = sat(y + b);
            *(pd1++) = sat(y - g);
            *(pd1++) = sat(y + r);
            ++pd1;

            y = *(py2++);
            *(pd2++) = sat(y + b);
            *(pd2++) = sat(y - g);
            *(pd2++) = sat(y + r);
            ++pd2;

            y = *(py2++);
            *(pd2++) = sat(y + b);
            *(pd2++) = sat(y - g);
            *(pd2++) = sat(y + r);
            ++pd2;
        }
        py1 = py2;
        py2 += W;
        pd1 = pd2;
        pd2 += W*4;
    }
}

void cvt3(const unsigned char * restrict py1, unsigned char * restrict pd1)
{
    const unsigned char *py2 = py1 + W;
    const unsigned char *pu = py1 + Y_SIZE;
    const unsigned char *pv = pu + U_SIZE;
    unsigned char *pd2 = pd1 + W*3;

    int y, u, v;
    int b, g, r;
    int i, j;
    for (i=0; i<H_2; ++i) {
        for (j=0; j<W_2; ++j) {
            u = *(pu++) - 128;
            v = *(pv++) - 128;

            b = (454*u) >> 8;
            g = (183*v + 88*u) >> 8;
            r = (359*v) >> 8;

            y = *(py1++);
            *(pd1++) = sat(y + r);
            *(pd1++) = sat(y - g);
            *(pd1++) = sat(y + b);

            y = *(py1++);
            *(pd1++) = sat(y + r);
            *(pd1++) = sat(y - g);
            *(pd1++) = sat(y + b);


            y = *(py2++);
            *(pd2++) = sat(y + r);
            *(pd2++) = sat(y - g);
            *(pd2++) = sat(y + b);


            y = *(py2++);
            *(pd2++) = sat(y + r);
            *(pd2++) = sat(y - g);
            *(pd2++) = sat(y + b);

        }
        py1 = py2;
        py2 += W;
        pd1 = pd2;
        pd2 += W*3;
    }
}


void cvtm(const unsigned char * restrict py1, unsigned char * restrict pd1, const int * restrict mask, int c)
{
    const unsigned char *py2 = py1 + W;
    const unsigned char *pu = py1 + Y_SIZE;
    const unsigned char *pv = pu + U_SIZE;
    unsigned char *pd2 = pd1 + W*4;

    int y, u, v;
    int b, g, r;
    int i, j;
    for (i=0; i<H_2; ++i) {
        for (j=0; j<W_2; ++j) {
            if (*mask == c) {
                ++pu;
                ++pv;
                py1 += 2;
                py2 += 2;
                *(pd1++) = 0;
                *(pd1++) = 255;
                *(pd1++) = 0;
                ++pd1;

                *(pd1++) = 0;
                *(pd1++) = 255;
                *(pd1++) = 0;
                ++pd1;

                *(pd2++) = 0;
                *(pd2++) = 255;
                *(pd2++) = 0;
                ++pd2;

                *(pd2++) = 0;
                *(pd2++) = 255;
                *(pd2++) = 0;
                ++pd2;
            } else if (*mask) {
                ++pu;
                ++pv;
                py1 += 2;
                py2 += 2;
                *(pd1++) = 255;
                *(pd1++) = 255;
                *(pd1++) = 255;
                ++pd1;

                *(pd1++) = 255;
                *(pd1++) = 255;
                *(pd1++) = 255;
                ++pd1;

                *(pd2++) = 255;
                *(pd2++) = 255;
                *(pd2++) = 255;
                ++pd2;

                *(pd2++) = 255;
                *(pd2++) = 255;
                *(pd2++) = 255;
                ++pd2;
            } else {
                u = *(pu++) - 128;
                v = *(pv++) - 128;

                b = (454*u) >> 8;
                g = (183*v + 88*u) >> 8;
                r = (359*v) >> 8;

                y = *(py1++);
                *(pd1++) = sat(y + b);
                *(pd1++) = sat(y - g);
                *(pd1++) = sat(y + r);
                ++pd1;

                y = *(py1++);
                *(pd1++) = sat(y + b);
                *(pd1++) = sat(y - g);
                *(pd1++) = sat(y + r);
                ++pd1;

                y = *(py2++);
                *(pd2++) = sat(y + b);
                *(pd2++) = sat(y - g);
                *(pd2++) = sat(y + r);
                ++pd2;

                y = *(py2++);
                *(pd2++) = sat(y + b);
                *(pd2++) = sat(y - g);
                *(pd2++) = sat(y + r);
                ++pd2;
            }
            ++mask;
        }
        py1 = py2;
        py2 += W;
        pd1 = pd2;
        pd2 += W*4;
        ++mask;
    }
}

void cvtc(const unsigned char * restrict py1, unsigned char * restrict pd1, const int * restrict mask, const int (* restrict rgb)[3])
{
    const unsigned char *py2 = py1 + W;
    const unsigned char *pu = py1 + Y_SIZE;
    const unsigned char *pv = pu + U_SIZE;
    unsigned char *pd2 = pd1 + W*4;
    int y, u, v;
    int b, g, r;
    int i, j;
    for (i=0; i<H_2; ++i) {
        for (j=0; j<W_2; ++j) {
            if (*mask) {
                ++pu;
                ++pv;
                py1 += 2;
                py2 += 2;

                b = rgb[*mask][0];
                g = rgb[*mask][1];
                r = rgb[*mask][2];

                *(pd1++) = b;
                *(pd1++) = g;
                *(pd1++) = r;
                ++pd1;

                *(pd1++) = b;
                *(pd1++) = g;
                *(pd1++) = r;
                ++pd1;

                *(pd2++) = b;
                *(pd2++) = g;
                *(pd2++) = r;
                ++pd2;

                *(pd2++) = b;
                *(pd2++) = g;
                *(pd2++) = r;
                ++pd2;
            } else {
                u = *(pu++) - 128;
                v = *(pv++) - 128;

                b = (454*u) >> 8;
                g = (183*v + 88*u) >> 8;
                r = (359*v) >> 8;

                y = *(py1++);
                *(pd1++) = sat(y + b);
                *(pd1++) = sat(y - g);
                *(pd1++) = sat(y + r);
                ++pd1;

                y = *(py1++);
                *(pd1++) = sat(y + b);
                *(pd1++) = sat(y - g);
                *(pd1++) = sat(y + r);
                ++pd1;

                y = *(py2++);
                *(pd2++) = sat(y + b);
                *(pd2++) = sat(y - g);
                *(pd2++) = sat(y + r);
                ++pd2;

                y = *(py2++);
                *(pd2++) = sat(y + b);
                *(pd2++) = sat(y - g);
                *(pd2++) = sat(y + r);
                ++pd2;
            }
            ++mask;
        }
        py1 = py2;
        py2 += W;
        pd1 = pd2;
        pd2 += W*4;
        ++mask;
    }
}


void cvtc3(const unsigned char * restrict py1, unsigned char * restrict pd1,
           const int * restrict mask, const int (* restrict rgb)[3])
{
    const unsigned char *py2 = py1 + W;
    const unsigned char *pu = py1 + Y_SIZE;
    const unsigned char *pv = pu + U_SIZE;
    unsigned char *pd2 = pd1 + W*3;

    int y, u, v;
    int b, g, r;
    int i, j;
    for (i=0; i<H_2; ++i) {
        for (j=0; j<W_2; ++j) {
            if (*mask) {
                ++pu;
                ++pv;
                py1 += 2;
                py2 += 2;

                b = rgb[*mask][0];
                g = rgb[*mask][1];
                r = rgb[*mask][2];

                *(pd1++) = r;
                *(pd1++) = g;
                *(pd1++) = b;

                *(pd1++) = r;
                *(pd1++) = g;
                *(pd1++) = b;

                *(pd2++) = r;
                *(pd2++) = g;
                *(pd2++) = b;

                *(pd2++) = r;
                *(pd2++) = g;
                *(pd2++) = b;
            } else {
                u = *(pu++) - 128;
                v = *(pv++) - 128;

                b = (454*u) >> 8;
                g = (183*v + 88*u) >> 8;
                r = (359*v) >> 8;

                y = *(py1++);
                *(pd1++) = sat(y + r);
                *(pd1++) = sat(y - g);
                *(pd1++) = sat(y + b);

                y = *(py1++);
                *(pd1++) = sat(y + r);
                *(pd1++) = sat(y - g);
                *(pd1++) = sat(y + b);


                y = *(py2++);
                *(pd2++) = sat(y + r);
                *(pd2++) = sat(y - g);
                *(pd2++) = sat(y + b);


                y = *(py2++);
                *(pd2++) = sat(y + r);
                *(pd2++) = sat(y - g);
                *(pd2++) = sat(y + b);

            }
            ++mask;
        }
        py1 = py2;
        py2 += W;
        pd1 = pd2;
        pd2 += W*3;
        ++mask;
    }
}

