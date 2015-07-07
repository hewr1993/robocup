#ifndef CVT_H
#define CVT_H

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

    void cvt(const unsigned char * restrict src, unsigned char * restrict dst);
    void cvt3(const unsigned char * restrict src, unsigned char * restrict dst);
    void cvtm(const unsigned char * restrict src, unsigned char * restrict dst, const int * restrict mask, int c);
    void cvtc(const unsigned char * restrict src, unsigned char * restrict dst, const int * restrict mask, const int (* restrict rgb)[3]);
    void cvtc3(const unsigned char * restrict py1, unsigned char * restrict pd1,
               const int * restrict mask, const int (* restrict rgb)[3]);

#ifdef __cplusplus
}
#endif

#endif

