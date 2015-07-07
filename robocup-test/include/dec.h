#ifndef DEC_H
#define DEC_H

#ifdef __cplusplus
extern "C" {
#endif

    void dec_init();
    void dec_process(unsigned char *src, unsigned char *dst);

#ifdef __cplusplus
}
#endif

#endif

