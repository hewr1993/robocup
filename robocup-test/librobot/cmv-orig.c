#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmv.h"

static inline int color_c(int n)
{
    if (!n) return 0;
    n &= -n;
    int i = 0;
    for (; n>>=1; ++i);
    return i;
}

static inline void set_bits(int *p, int x1, int x2, int b)
{
    if (x1 < 0 || x1 > 255 || x2 < 0 || x2 > 255 || x1 > x2) {
        fprintf(stderr, "threshold range error: %d %d\n", x1, x2);
        exit(-1);
    }
    for (; x1<=x2; ++x1) p[x1] |= b;
}

void cmv_init(struct cmv_data * restrict data, const char * restrict filename)
{
    FILE *in = fopen(filename, "r");
    if (in == NULL) {
        perror(filename);
        exit(-1);
    }

    int c;
    if (fscanf(in, "%d", &c) != 1) {
        fprintf(stderr, "read color_n error\n");
        exit(-1);
    }
    if (c <= 0 || c > INT_BITS) {
        fprintf(stderr, "too many colors: %d\n", c);
        exit(-1);
    }
    data->color_n = c;

    if ((data->th = malloc(c * 6 * sizeof(int))) == NULL) {
        perror("malloc");
        exit(-1);
    }

    memset(data->thresh, 0, sizeof(data->thresh));
    int b = 1;
    int i;
    int *t;
    for (i=0; i<c; ++i) {
        t = data->th[i];
        if (fscanf(in, "%d %d %d %d %d %d", t, t+1, t+2, t+3, t+4, t+5) != 6) {
            fprintf(stderr, "read threshold error\n");
            exit(-1);
        }
        set_bits(data->thresh[0], t[0], t[1], b);
        set_bits(data->thresh[1], t[2], t[3], b);
        set_bits(data->thresh[2], t[4], t[5], b);
        b <<= 1;
    }

    if ((data->remap = malloc(c * sizeof(int))) == NULL) {
        perror("malloc");
        exit(-1);
    }
    for (i=0; i<c; ++i) {
        if (fscanf(in, "%d", &b) != 1) {
            fprintf(stderr, "read remap error\n");
            exit(-1);
        }
        if (b > c || b < 0) {
            fprintf(stderr, "invalid remap: %d\n", b);
            exit(-1);
        }
        data->remap[i] = b;
    }

    if ((data->clist = malloc(c * sizeof(int))) == NULL) {
        perror("malloc");
        exit(-1);
    }
    if ((data->cmax = malloc(c * sizeof(struct reg_t *))) == NULL) {
        perror("malloc");
        exit(-1);
    }

    if ((data->rgb = malloc((c+1) * 3 * sizeof(int))) == NULL) {
        perror("malloc");
        exit(-1);
    }
    if (fscanf(in, "%d", &c) != 1) {
        fprintf(stderr, "read rgb_n error\n");
        exit(-1);
    }
    if (c > data->color_n + 1 || c < 0) {
        fprintf(stderr, "invalid rgb_n: %d\n", c);
        exit(-1);
    }
    for (i=0; i<c; ++i) {
        if (fscanf(in, "%d", &b) != 1) {
            fprintf(stderr, "read rgb_i error\n");
            exit(-1);
        }
        if (b > data->color_n || b < 0) {
            fprintf(stderr, "rgb_i invalid: %d\n", b);
            exit(-1);
        }
        t = data->rgb[b];
        if (fscanf(in, "%d %d %d", t, t+1, t+2) != 3) {
            fprintf(stderr, "read rgb error\n");
            exit(-1);
        }
        if (t[0] < 0 || t[0] > 255 || t[1] < 0 || t[1] > 255 || t[2] < 0 || t[2] > 255) {
            fprintf(stderr, "rgb invalid\n");
            exit(-1);
        }
    }

    if (fscanf(in, "%d", &c) != 1) {
        fprintf(stderr, "read size_min error\n");
        exit(-1);
    }
    if (c < 0) {
        fprintf(stderr, "size_min invalid: %d\n", c);
        exit(-1);
    }
    data->size_min = c;

    if (fscanf(in, "%d", &c) != 1) {
        fprintf(stderr, "read max_rle error\n");
        exit(-1);
    }
    if ((data->rle = malloc(c * sizeof(struct rle_t))) == NULL) {
        perror("malloc");
        exit(-1);
    }
    data->rle_max = c;

    if (fscanf(in, "%d", &c) != 1) {
        fprintf(stderr, "read max_reg error\n");
        exit(-1);
    }
    if ((data->reg = malloc(c * sizeof(struct reg_t))) == NULL) {
        perror("malloc");
        exit(-1);
    }
    data->reg_max = c;

    fclose(in);
}

static inline void classify(struct cmv_data * restrict data, const unsigned char * restrict py)
{
    const unsigned char *pu = py + Y_SIZE;
    const unsigned char *pv = pu + U_SIZE;
    int *m = data->map;

    int i, j;
    for (j=0; j<H_2; ++j) {
        for (i=0; i<W_2; ++i) {
            *(m++) = data->remap[color_c(data->thresh[0][*py] & data->thresh[1][*(pu++)] & data->thresh[2][*(pv++)])];
            py += 2;
        }
        *(m++) = -1;
        py += W;
    }
}

static inline void encode(struct cmv_data * data)
{
    struct rle_t *r = data->rle;
    int *l = data->map;
    int *p = data->map;

    int y = 0;
    for (;;) {
        int *b = p;
        int t = *p;
        while (*(++p) == t);

        if (t == -1) {
            l += (W_2 + 1); // next line
            r->color = 0;  // change color_end to 0
            r->x2 = (W_2 + 1);
            ++r;
            if (++y == H_2) break;
        } else if (t) { // [x1, x2), true 0 is not added
            r->color = t;
            r->x1 = b - l;
            r->x2 = p - l;
            r->parent = r;
            ++r;
        }
    }

    data->rle_end = r;
    y = r - data->rle;
    if (y > data->rle_max) {
        fprintf(stderr, "rle overflow: %d\n", y);
        exit(-1);
    }
}

static inline void connect(struct cmv_data * data)
{
    // l1: first line
    // l2: second line
    struct rle_t *l1 = data->rle;
    struct rle_t *l2 = data->rle;
    while ((l2++)->color);

    int y = 1;
    for (;;) {
        if (l1->color == l2->color) {
            if (!l1->color) {
                if (++y == H_2)  break;
            } else if (l2->x1 >= l1->x1 && l2->x1 < l1->x2) { // safe
                l2->parent = l1->parent;
            } else if (l1->x1 >= l2->x1 && l1->x1 < l2->x2) {
                if (l1->parent > l2->parent) { // l2->parent is better
                    ((struct rle_t *)(l1->parent))->parent = l2->parent;
                } else if (l1->parent < l2->parent) { // l1->parent is better
                    ((struct rle_t *)(l2->parent))->parent = l1->parent;
                    l2->parent = l1->parent; // base is important
                }
            }
        }

        int t = l2->x2 - l1->x2;
        if (t >= 0) ++l1;
        if (t <= 0) ++l2;
    }

    for (l1 = data->rle; l1<data->rle_end; ++l1) {
        if (l1->color) {
            l1->parent = ((struct rle_t *)(l1->parent))->parent;
        }
    }
}

static inline int max(int a, int b)
{
    return(a > b ? a : b);
}

static inline int min(int a, int b)
{
    return(a < b ? a : b);
}

static inline int sum(int x, int dx)
{
    x <<= 1;
    return(dx * (x + dx - 1) >> 1);
}

static inline void extract(struct cmv_data * data)
{
    struct rle_t *r = data->rle;
    struct reg_t *g = data->reg;

    int y = 0;
    for (;;) {
        if (r->color) {
            if (r->parent == r) {
                int t = r->x2 - r->x1;
                r->parent = g;
                g->color = r->color;
                g->x1 = r->x1;
                g->x2 = r->x2;
                g->size = t;
                g->y1 = y;
                g->y2 = y;
                g->cx = sum(r->x1, t);
                g->cy = y * t;
                ++g;
            } else {
                int t = r->x2 - r->x1;
                struct reg_t *tg = (struct reg_t *)((struct rle_t *)(r->parent))->parent;
                tg->x1 = min(tg->x1, r->x1);
                tg->x2 = max(tg->x2, r->x2);
                tg->size += t;
                tg->y2 = y;
                tg->cx += sum(r->x1, t);
                tg->cy += y * t;
            }
        } else {
            if (++y == H_2) break;
        }
        ++r;
    }

    data->reg_end = g;
    y = g - data->reg;
    if (y > data->reg_max) {
        fprintf(stderr, "reg overflow: %d\n", y);
        exit(-1);
    }
}

static inline void separate(struct cmv_data * data)
{
    memset(data->clist, 0, sizeof(data->color_n * sizeof(int)));
    struct reg_t *g = data->reg;

    for (; g<data->reg_end; ++g) {
        int c = g->size;
        if (c > data->size_min) {
            g->cx /= c;
            g->cy /= c;
            c = g->color - 1;
            //cmv_ctable[c][cmv_clist[c]] = g;
            if (!data->clist[c]) data->cmax[c] = g;
            else if (g->size > data->cmax[c]->size) data->cmax[c] = g;
            ++data->clist[c];
        }
    }

    /*
    int i = 0;
    for (; i<cdata->color_n; ++i) {
    	if (cdata->clist[i] > MAX_RPC) {
    		fprintf(stderr, "rpc overflow: %d\n", cdata->clist[i]);
    		exit(-1);
    	}
    }
    */
}

void cmv_process(struct cmv_data * restrict data, const unsigned char * restrict src)
{
    classify(data, src);
    encode(data);
    connect(data);
    extract(data);
    separate(data);
}

void cmv_reinit(struct cmv_data * restrict data, int ch, int i, int target, int type)
{
    if (ch < 0 || ch > 2) {
        fprintf(stderr, "channel error\n");
        exit(-1);
    }

    i = 1 << i;
    if (type) data->thresh[ch][target] |= i;
    else data->thresh[ch][target] &= ~i;
}

