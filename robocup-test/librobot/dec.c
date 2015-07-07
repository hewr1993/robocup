#include "global.h"
#include "dec.h"
#include "table.h"

#include <string.h>

struct pwc_dec23_private {

    unsigned char xx,yy,zz,zzmask;

    unsigned char table_0004[2*0x4000];
    unsigned char table_8004[2*0x1000];
    unsigned int  table_a004[256*12];

    unsigned char table_d004[8*256];
    unsigned int  table_d800[256];
    unsigned int  table_dc00[256];
};

static struct pwc_dec23_private pdev;

static inline void fill_table_a000()
{

    unsigned int temp_values[12] = {
        0xFFAD9B00, 0xFFDDEE00, 0x00221200, 0x00526500,
        0xFFC21E00, 0x003DE200, 0xFF924B80, 0xFFD2A300,
        0x002D5D00, 0x006DB480, 0xFFED3E00, 0x0012C200
    };
    unsigned int values_derivated[12] = {
        0x0000A4CA, 0x00004424, 0xFFFFBBDC, 0xFFFF5B36,
        0x00007BC4, 0xFFFF843C, 0x0000DB69, 0x00005ABA,
        0xFFFFA546, 0xFFFF2497, 0x00002584, 0xFFFFDA7C
    };

    unsigned int *p = pdev.table_a004;
    int i, j;
    for (i = 0; i < 256; i++) {
        for (j = 0; j < 12; j++) {
            *p++ = temp_values[j];
            temp_values[j] += values_derivated[j];
        }
    }
}

static inline void fill_table_d000()
{

    memset(pdev.table_d004, 0, 256);

    unsigned char *p = pdev.table_d004 + 256;
    unsigned char bitpower = 2;
    int bit, byte;

    for (bit = 1; bit < 8; ++bit) {
        unsigned char mask = bitpower - 1;
        for (byte = 0; byte < 256; ++byte) {
            if (byte & bitpower)
                *p = -(byte & mask);
            else
                *p = (byte & mask);
            ++p;
        }
        bitpower <<= 1;
    }
}

static inline void fill_table_color(unsigned int *romtab)
{

    unsigned char shift[16] = {0, 15, 15, 12, 12, 12, 9, 9, 9, 9, 6, 6, 6, 3, 3, 0};
    int i, dl, j, k;

    for (i = 0; i < 2; ++i) {
        unsigned int *table = romtab + (i << 7);

        for (dl = 0; dl < 16; ++dl) {
            unsigned char *p0 = pdev.table_0004 + (i << 14) + (dl << 10);
            unsigned char *p8 = pdev.table_8004 + (i << 12) + (dl << 8);

            for (j = 0; j < 8; ++j, ++table, p0 += 128) {
                for (k = 0; k < 16; ++k) {
                    int bit;
                    if (k == 0) {
                        bit = 1;
                        *p8 = 8;
                        ++p8;
                    } else {
                        bit = (*table >> shift[k]) & 7;
                        *p8 = j - bit;
                        ++p8;
                    }

                    *p8 = bit;
                    ++p8;

                    int pw = 1 << bit;
                    p0[k] = (pw) + 0x80;
                    p0[k + 0x10] = (2 * pw) + 0x80;
                    p0[k + 0x20] = (3 * pw) + 0x80;
                    p0[k + 0x30] = (4 * pw) + 0x80;
                    p0[k + 0x40] = (-pw) + 0x80;
                    p0[k + 0x50] = (2 * -pw) + 0x80;
                    p0[k + 0x60] = (3 * -pw) + 0x80;
                    p0[k + 0x70] = (4 * -pw) + 0x80;
                }
            }
        }
    }
}

static inline void fill_table_dc00_d800()
{

    unsigned int *pdc00 = pdev.table_dc00;
    unsigned int *pd800 = pdev.table_d800;
    unsigned int offset1 = 0x4000;
    unsigned int offset2 = 0;
    int i = 0;

    for (; i < 256; ++i, offset1 += 0x7BC4, offset2 += 0x7BC4) {

        unsigned int msb = offset1 >> 15;
        if (msb > 255) {
            msb = 0;
        }

        *pdc00 = msb << 15;
        ++pdc00;
        *pd800 = offset2;
        ++pd800;
    }

}

void dec_init()
{

    int mode = MODE;
    switch (mode & 0x18) {
    case 0x10:
        pdev.zz = 8;
        break;
    case 0x08:
        pdev.zz = 7;
        break;
    default:
        pdev.zz = 6;
    }
    mode &= ~0x1f;
    mode <<= 3;

    fill_table_color((unsigned int *)KiaraRomTable + mode);

    pdev.xx = 8 - pdev.zz;
    pdev.yy = 15 - pdev.xx;
    pdev.zzmask = 0xFF >> pdev.xx;

    fill_table_dc00_d800();
    fill_table_a000();
    fill_table_d000();
}

/*
 * To manage the stream, we keep in a 32 bits variables,
 * the next bits in the stream. fill_reservoir() add to
 * the reservoir at least wanted nbits.
 *
 */
#define fill_nbits(reservoir,nbits_in_reservoir,stream,nbits_wanted) do { \
   while (nbits_in_reservoir<nbits_wanted) \
    { \
      reservoir |= (*(stream)++) << nbits_in_reservoir; \
      nbits_in_reservoir+=8; \
    } \
}  while(0);

#define get_nbits(reservoir,nbits_in_reservoir,stream,nbits_wanted,result) do { \
   fill_nbits(reservoir,nbits_in_reservoir,stream,nbits_wanted); \
   result = (reservoir) & ((1U<<nbits_wanted)-1); \
   reservoir >>= nbits_wanted; \
   nbits_in_reservoir -= nbits_wanted; \
}  while(0);

#define CLAMP(x) ((x)>255?255:((x)<0?0:x))

static inline void DecompressBand23(unsigned char *rawyuv, unsigned char *dest_y, unsigned char *dest_u, unsigned char *dest_v)
{

    unsigned int reservoir, nbits_in_reservoir;
    int first_4_bits;
    unsigned int bytes_per_channel;
    int passes;
    unsigned char *ptable0004, *ptable8004;

    int even_line;
    unsigned int temp_colors[16];
    int nblocks;

    unsigned char *stream;
    unsigned int offset_to_plane_u, offset_to_plane_v;

    int i;

    reservoir = 0;
    nbits_in_reservoir = 0;
    stream = rawyuv + 1;	/* The first byte of the stream is skipped */
    even_line = 1;

    get_nbits(reservoir, nbits_in_reservoir, stream, 4, first_4_bits);

    for (passes = 0; passes < 2; ++passes) {
        if (passes == 0) {
            bytes_per_channel = W;
            nblocks = W / 4;
        } else {
            /* Format planar: All Y, then all U, then all V */
            bytes_per_channel = W / 2;
            dest_y = dest_u;
            nblocks = W / 8;
        }

        offset_to_plane_u = bytes_per_channel * 2;
        offset_to_plane_v = bytes_per_channel * 3;

        while (nblocks-- > 0) {
            unsigned int gray_index;

            fill_nbits(reservoir, nbits_in_reservoir, stream, 16);
            gray_index = reservoir & pdev.zzmask;
            reservoir >>= pdev.zz;
            nbits_in_reservoir -= pdev.zz;

            fill_nbits(reservoir, nbits_in_reservoir, stream, 2);

            if ((reservoir & 3) == 0) {
                reservoir >>= 2;
                nbits_in_reservoir -= 2;
                for (i = 0; i < 16; i++)
                    temp_colors[i] =
                        pdev.table_dc00[gray_index];

            } else {
                unsigned int channel_v, offset1;

                /* swap bit 0 and 2 of offset_OR */
                channel_v =
                    ((reservoir & 1) << 2) | (reservoir & 2) |
                    ((reservoir & 4) >> 2);
                reservoir >>= 3;
                nbits_in_reservoir -= 3;

                for (i = 0; i < 16; i++)
                    temp_colors[i] =
                        pdev.table_d800[gray_index];

                ptable0004 =
                    pdev.table_0004 + (passes * 16384) +
                    (first_4_bits * 1024) + (channel_v * 128);
                ptable8004 =
                    pdev.table_8004 + (passes * 4096) +
                    (first_4_bits * 256) + (channel_v * 32);

                offset1 = 0;
                while (1) {
                    unsigned int index_in_table_ops, op,
                    rows = 0;
                    fill_nbits(reservoir,
                               nbits_in_reservoir, stream,
                               16);

                    /* mode is 0,1 or 2 */
                    index_in_table_ops = (reservoir & 0x3F);
                    op = table_ops[index_in_table_ops * 4];
                    if (op == 2) {
                        reservoir >>= 2;
                        nbits_in_reservoir -= 2;
                        break;	/* exit the while(1) */
                    }
                    if (op == 0) {
                        unsigned int shift;

                        offset1 =
                            (offset1 +
                             table_ops
                             [index_in_table_ops * 4 +
                              2]) & 0x0F;
                        shift =
                            table_ops[index_in_table_ops
                                      * 4 + 1];
                        reservoir >>= shift;
                        nbits_in_reservoir -= shift;
                        rows =
                            ptable0004[offset1 +
                                       table_ops
                                       [index_in_table_ops
                                        * 4 + 3]];
                    }
                    if (op == 1) {
                        /* 10bits [ xxxx xxxx yyyy 000 ]
                         * yyy => offset in the table8004
                         * xxx => offset in the tabled004
                         */
                        unsigned int mask, shift;
                        unsigned int col1, row1,
                        total_bits;

                        offset1 =
                            (offset1 +
                             ((reservoir >> 3) & 0x0F) +
                             1) & 0x0F;

                        col1 = (reservoir >> 7) & 0xFF;
                        row1 = ptable8004[offset1 * 2];

                        /* Bit mask table */
                        mask =
                            pdev.table_d004[(row1 << 8) +
                                            col1];
                        shift =
                            ptable8004[offset1 * 2 + 1];
                        rows =
                            ((mask << shift) +
                             0x80) & 0xFF;

                        total_bits = row1 + 8;
                        reservoir >>= total_bits;
                        nbits_in_reservoir -=
                            total_bits;
                    }
                    {
                        unsigned int *table_a004 = pdev.table_a004 + rows * 12;
                        unsigned int *poffset = MulIdx + offset1 * 16;	/* 64/4 (int) */
                        for (i = 0; i < 16; i++) {
                            temp_colors[i] +=
                                table_a004
                                [*poffset];
                            poffset++;
                        }
                    }
                }
            }

            if (passes == 0) {
                int *c = (int *)temp_colors;
                unsigned char *d;

                d = dest_y;
                for (i = 0; i < 4; i++, c++)
                    *d++ = CLAMP((*c) >> pdev.yy);

                d = dest_y + bytes_per_channel;
                for (i = 0; i < 4; i++, c++)
                    *d++ = CLAMP((*c) >> pdev.yy);

                d = dest_y + offset_to_plane_u;
                for (i = 0; i < 4; i++, c++)
                    *d++ = CLAMP((*c) >> pdev.yy);

                d = dest_y + offset_to_plane_v;
                for (i = 0; i < 4; i++, c++)
                    *d++ = CLAMP((*c) >> pdev.yy);

                dest_y += 4;
            } else if (passes == 1) {
                int *c1 = (int *)temp_colors;
                int *c2 = (int *)temp_colors + 4;
                unsigned char *d;

                d = dest_y;
                for (i = 0; i < 4; i++, c1++, c2++) {
                    *d++ = CLAMP((*c1) >> pdev.yy);
                    *d++ = CLAMP((*c2) >> pdev.yy);
                }
                c1 = (int *)temp_colors + 12;
                //c2 = temp_colors+8;
                d = dest_y + bytes_per_channel;
                for (i = 0; i < 4; i++, c1++, c2++) {
                    *d++ = CLAMP((*c1) >> pdev.yy);
                    *d++ = CLAMP((*c2) >> pdev.yy);
                }

                if (even_line) {	/* Each line, swap u/v */
                    even_line = 0;
                    dest_y = dest_v;
                    dest_u += 8;
                } else {
                    even_line = 1;
                    dest_y = dest_u;
                    dest_v += 8;
                }
            }

        }		/* end of while (nblocks-->0) */

    }			/* end of for (passes=0;passes<2;passes++) */
}

void dec_process(unsigned char *src, unsigned char *py)
{

    unsigned char *pu = py + Y_SIZE;
    unsigned char *pv = pu + U_SIZE;

    int i = 0;
    for (; i < (H/4); ++i) {
        DecompressBand23(src, py, pu, pv);
        src += BANDLEN;
        py += (W*4);
        pu += W;
        pv += W;
    }
}

