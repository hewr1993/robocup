#ifndef CALIB_H
#define CALIB_H

#include <stdio.h>
#include <stdlib.h>
#include "clapack.h"

inline void calib(const char * const filename, double b[])
{
    double sx, sy, wx, wy;
    double a[64];
    int ipiv[8];

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        exit(-1);
    }

    int i;
    for (i=0; i<4; ++i) {
        if (fscanf(fp, "%lf %lf %lf %lf", &wx, &wy, &sx, &sy) != 4) {
            fprintf(stderr, "fscanf error\n");
            exit(-1);
        }

        a[8*i] = sx;
        a[8*i+1] = sy;
        a[8*i+2] = 1;
        a[8*i+3] = 0;
        a[8*i+4] = 0;
        a[8*i+5] = 0;
        a[8*i+6] = -sx * wx;
        a[8*i+7] = -sy * wx;

        a[8*i+32] = 0;
        a[8*i+33] = 0;
        a[8*i+34] = 0;
        a[8*i+35] = sx;
        a[8*i+36] = sy;
        a[8*i+37] = 1;
        a[8*i+38] = -sx * wy;
        a[8*i+39] = -sy * wy;

        b[i] = wx;
        b[i+4] = wy;
    }

    fclose(fp);

    if (clapack_dgesv(CblasRowMajor, 8, 1, a, 8, ipiv, b, 8) != 0) {
        fprintf(stderr, "dgesv error\n");
        exit(-1);
    }
}

#endif

