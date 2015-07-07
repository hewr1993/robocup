#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "../my_utils.h"
#include "cmv.h"

#define BALL_H 17
#define ROBO_H 24

const char *transfile = "translate.conf";
static inline int color_remap(struct cmv_data * data, int n)
{
	if(!n)
		return 0;
	int *p = data->remap;
	n &= -n;
	for(; n>>=1; ++p);
	return *p;
}

static inline void set_bits(int *p, int x1, int x2, int b) {
	if (x1 < 0 || x1 > 255 || x2 < 0 || x2 > 255 || x1 > x2) {
		mylogfd(2, "[Version|Cmv|set_bit] Threshold range error: %d %d\n", x1, x2);
		exit(-1);
	}
	for (; x1<=x2; ++x1) p[x1] |= b;
}

void cmv_init(struct cmv_data * restrict data, const char * restrict filename)
{
	FILE *in = fopen(filename, "r");
	if (in == NULL) {
		mylogfd(2, "[Version|Cmv|cmv_init] Can't open the file : %s\n", filename);
		perror(filename);
		exit(-1);
	}

	int c;
	if (fscanf(in, "%d", &c) != 1) {
		mylogfd(2, "[Version|Cmv|cmv_init] Read color_n error\n");
		exit(-1);
	}
	if (c <= 0 || c > INT_BITS) {
		mylogfd(2, "[Version|Cmv|cmv_init] Too many colors: %d\n", c);
		exit(-1);
	}
	data->color_n = c;
	if ((data->th = malloc(c * 6 * sizeof(int))) == NULL) {
		mylogfd(2, "[Version|Cmv|cmv_init] Malloc error\n");
		perror("malloc");
		exit(-1);
	}

	memset(data->thresh, 0, sizeof(data->thresh));
	int b = 1,i;
	int *t;
	for (i=0; i<c; ++i) {
		t = data->th[i];
		if (fscanf(in, "%d %d %d %d %d %d", t, t+1, t+2, t+3, t+4, t+5) != 6) {
			mylogfd(2, "[Version|Cmv|cmv_init] read threshold error\n");
			exit(-1);
		}
		set_bits(data->thresh[0], t[0], t[1], b);
		set_bits(data->thresh[1], t[2], t[3], b);
		set_bits(data->thresh[2], t[4], t[5], b);
		b <<= 1;
	}

	if ((data->remap = malloc(c * sizeof(int))) == NULL) {
		mylogfd(2, "[Version|Cmv|cmv_init] Malloc error\n");
		perror("malloc");
		exit(-1);
	}
	for (i=0; i<c; ++i) {
		if (fscanf(in, "%d", &b) != 1) {
			mylogfd(2, "[Version|Cmv|cmv_init] read remap error\n");
			exit(-1);
		}
		if (b > c || b < 0) {
			mylogfd(2, "[Version|Cmv|cmv_init] invalid remap: %d\n", b);
			exit(-1);
		}
		data->remap[i] = b;
	}

	//I make some change: by Eric
	//if ((data->clist = malloc(c * sizeof(int))) == NULL) {
	if ((data->clist = malloc(c * sizeof(int ))) == NULL) {
		mylogfd(2, "[Version|Cmv|cmv_init] Malloc error\n");
		perror("malloc");
		exit(-1);
	}
	if ((data->cmax = malloc(c * sizeof(struct reg_t *))) == NULL) {
		mylogfd(2, "[Version|Cmv|cmv_init] Malloc error\n");
		perror("malloc");
		exit(-1);
	}

	if ((data->rgb = malloc((c+1) * 3 * sizeof(int))) == NULL) {
		mylogfd(2, "[Version|Cmv|cmv_init] Malloc error\n");
		perror("malloc");
		exit(-1);
	}
	if (fscanf(in, "%d", &c) != 1) {
		mylogfd(2, "[Version|Cmv|cmv_init] Read rgb_n error\n");
		exit(-1);
	}
	if (c > data->color_n + 1 || c < 0) {
		mylogfd(2, "[Version|Cmv|cmv_init] Invalid rgb_n: %d\n", c);
		exit(-1);
	}

	data->rgb_n = c;
	for (i=0; i<c; ++i) {
		if (fscanf(in, "%d", &b) != 1) {
			mylogfd(2, "[Version|Cmv|cmv_init] Read rgb_i error\n");
			exit(-1);
		}
		if (b > data->color_n || b < 0) {
			mylogfd(2, "[Version|Cmv|cmv_init] Rgb_i invalid: %d\n", b);
			exit(-1);
		}
		t = data->rgb[b];
		if (fscanf(in, "%d %d %d", t, t+1, t+2) != 3) {
			mylogfd(2, "[Version|Cmv|cmv_init] Read rgb error\n");
			exit(-1);
		}
		if (t[0] < 0 || t[0] > 255 || t[1] < 0 || t[1] > 255 || t[2] < 0 || t[2] > 255) {
			mylogfd(2, "[Version|Cmv|cmv_init] Rgb invalid\n");
			exit(-1);
		}
	}

	if (fscanf(in, "%d", &c) != 1) {
		mylogfd(2, "[Version|Cmv|cmv_init] read size_min error\n");
		exit(-1);
	}
	if (c < 0) {
		mylogfd(2, "[Version|Cmv|cmv_init] size_min invalid: %d\n", c);
		exit(-1);
	}
	data->size_min = c;

	if (fscanf(in, "%d", &c) != 1) {
		mylogfd(2, "[Version|Cmv|cmv_init]  Read max_rle error\n");
		exit(-1);
	}
	if ((data->rle = malloc(c * sizeof(struct rle_t))) == NULL) {
		mylogfd(2, "[Version|Cmv|cmv_init] Malloc error\n");
		perror("malloc");
		exit(-1);
	}
	data->rle_max = c;

	if (fscanf(in, "%d", &c) != 1) {
		mylogfd(2, "[Version|Cmv|cmv_init] Read max_reg error\n");
		exit(-1);
	}
	if ((data->reg = malloc(c * sizeof(struct reg_t))) == NULL) {
		mylogfd(2, "[Version|Cmv|cmv_init] Malloc error\n");
		perror("malloc");
		exit(-1);
	}
	data->reg_max = c;
	fclose(in);
}

//
// name: 按照阈值，对各个点进行分类
// @param
// @return
static inline void classify(struct cmv_data * restrict data, const unsigned char * restrict py)
{
	const unsigned char *pu = py + Y_SIZE;
	const unsigned char *pv = pu + U_SIZE;
	int *m = data->map;

	//存储格式：一维数组，按行存储，每一行最后添加-1
	int i, j;
	for (j=0; j<H_2; ++j) {
		for (i=0; i<W_2; ++i) {·
			*(m++) = color_remap(data, data->thresh[0][*py] &
			data->thresh[1][*(pu++)] & data->thresh[2][*(pv++)]);
			py += 2;
		}
		*(m++) = -1;
		py += W;
	}
}

//
// name: 将图像按照分类，按行进行区域化，一行中连续的相同类别组成了一个区域。
// @param
// @return
static inline void encode(struct cmv_data * data)
{
	struct rle_t *r = data->rle;
	int *l = data->map;
	int *p = data->map;
	//mylogfd(-1, "[Version|Cmv|encode] start loop\n");
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
			if (++y == H_2)
				break;
		}
		else if (t) { // [x1, x2), true 0 is not added
			r->color = t;
			r->x1 = b - l;
			r->x2 = p - l;
			r->parent = r;
			++r;
		}
	}
	//mylogfd(-1, "[Version|Cmv|encode] end loop\n");
	data->rle_end = r;
	y = r - data->rle;
	if (y > data->rle_max) {
		mylogfd(2, "[Version|Cmv|encode] rle overflow: %d\n", y);
		exit(-1);
	}
}

//
// name: 将一个联通域通过一个parent相连
// @param
// @return
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
				if (++y == H_2)
					break;
			}
			else if (l2->x1 >= l1->x1 && l2->x1 < l1->x2) { // safe
				l2->parent = l1->parent;
			}
			else if (l1->x1 >= l2->x1 && l1->x1 < l2->x2) {
				if (l1->parent > l2->parent) { // l2->parent is better
					((struct rle_t *)(l1->parent))->parent = l2->parent;
				}
				else if(l1->parent < l2->parent) { // l1->parent is better
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

static inline int sum(int x, int dx)
{
	x <<= 1;
	return(dx * (x + dx - 1) >> 1);
}

//
// name: 计算出一个一个联通域的外接矩形包信息
// @param
// @return
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
			}
			else {
				int t = r->x2 - r->x1;
				struct reg_t *tg = (struct reg_t *)((struct rle_t *)(r->parent))->parent;
				tg->x1 = min(tg->x1, r->x1);
				tg->x2 = max(tg->x2, r->x2);
				tg->size += t;
				tg->y2 = y;
				tg->cx += sum(r->x1, t);
				tg->cy += y * t;
			}
		}
		else {
			if (++y == H_2)
				break;
		}
		++r;
	}

	data->reg_end = g;
	y = g - data->reg;
	if (y > data->reg_max) {
		mylogfd(2, "[Version|Cmv|extract] reg overflow: %d\n", y);
		exit(-1);
	}
}

//By 彭瑾玥
//1.找出程序里调用了ball_filter的地方，传参struct cmv_data * data
//2.调参数xxx
//为了计算红色率，需要得到map信息，故在此加入参数 struct cmv_data * data
int ball_filter(struct reg_t* g, struct cmv_data * data) {
    if (g->color != COLOR_B + 1)
        return 1;
    if (g->y2 <= INVALID_BALL_Y)
        return 0;
    if (g->y2 >= BOTTOM_LINE)
        return 1;
    double temp = (double)(g->x2 - g->x1) / (g->y2 - g->y1);
    //外包矩形的斜率要满足一定范围，不能太偏，原框架
    if (temp < 0.6 || temp > 1.5)//矩形斜率
        return 0;
    //nonsense
    if (g->size < 60)//矩形面积
        return 0;
	
	//By 彭瑾玥
	//计算矩阵中的红色率
	double red_ratio = 0.0;
	int* m = data->map;
	int i=0;
	int j=0;
	int red_count = 0;
	for (i=g->x1; i++; i<=g->x2)
		for (j=g->y1; j++; j<=g->y2)
		{
			//得到点(i,j)在map中对应的位置
			int pos = (W_2+1)*i + j;
			
			if (m[pos] == COLOR_B)
				red_count++;
		}
	red_ratio = red_count * 1.0 / g->size;
	
	//XXX为红色率的下限，需要调参数
	
	if (red_ratio < 100)
		return 0;
		
		
    return 1;
}

void check_for_merge(struct reg_t *cmax, struct reg_t *g) {
	if(g->size < 80 || g-> y2 <= cmax->y2)
		return;
	if((g->x1 > cmax-> x1 && g->x1 < cmax-> x2
			&& g->y1 > cmax-> y1 && g->y1 < cmax-> y2)
			|| (abs(g->x1-cmax->x1) < 3 &&
					abs(g->y1-cmax->y2) < 3)) {
						cmax->y2 = g->y2;
						cmax->x2 = max(cmax->x2, g->x2);
					}
}

//
// name: 遍历找到的每一块连通区域，将一种颜色最大的联通区域找到
// @param
// @return
static inline void separate(struct cmv_data * data) {
	//change by :Eric
	memset(data->clist, 0, data->color_n * sizeof(int ));
	memset(data->cmax, 0, data->color_n * sizeof(struct reg_t *));
	struct reg_t *g = data->reg;
	int count=0;
	for (; g<data->reg_end; ++g) {
		//遍历找到的每一块连通区域，将一种颜色最大的联通区域找到
		int size = g->size;
		int c;
		count++;
		if (size > data->size_min && ball_filter(g,data)) {
			g->cx /= size;
			g->cy /= size;
			c = g->color - 1;
			if (!data->clist[c]){
                data->cmax[c] = g;
			}
			else if (g->size > data->cmax[c]->size) {
                data->cmax[c] = g;
			}
            else if(c == COLOR_R)
            		check_for_merge(data->cmax[c], g);
			++data->clist[c];
		}
	}
}



void fill(struct cmv_data *data, int color);

void cmv_process(struct cmv_data * restrict data, const unsigned char * restrict src) {

	//mylogfd(logfd, "in robocup_program %d\n", data->color_n);
	//根据src以及data中的阈值，将分类结果存入data
	//mylogfd(-1, "[Version|Cmv|cmv_pro] start class\n");
	//mylogfd(-1, "[Version|Cmv|cmv_pro] color_size: %d\n",data->color_n);

	classify(data, src);
	//将图像按照分类，按行进行区域化，一行中连续的相同类别组成了一个区域。
	//mylogfd(-1, "[Version|Cmv|cmv_pro] start encode\n");
	encode(data);
	//将一个联通域通过一个parent相连
	//mylogfd(-1, "[Version|Cmv|cmv_pro] start connect\n");
	connect(data);
	//计算出一个一个联通域的外接矩形包信息
	//mylogfd(-1, "[Version|Cmv|cmv_pro] start extract\n");
	extract(data);
	//遍历找到的每一块连通区域，将一种颜色最大的联通区域找到
	//mylogfd(-1, "[Version|Cmv|cmv_pro] start separate\n");
	separate(data);
}

void cmv_reinit(struct cmv_data * restrict data, int ch, int i, int target, int type) {
	if (ch < 0 || ch > 2) {
		mylogfd(2, "[Version|Cmv|cmv_reinit] Channel error\n");
		exit(-1);
	}

	i = 1 << i;
	if (type) data->thresh[ch][target] |= i;
	else data->thresh[ch][target] &= ~i;
}

inline int cround(int map[][W_2 + 1], int x, int y, int c) {
	int sum;

	sum = (map[y-1][x-1] == c) + (map[y-1][x] == c) +
		(map[y-1][x+1] == c) + (map[y][x-1] == c)
		+ (map[y][x+1] == c) + (map[y+1][x-1] == c) +
		(map[y+1][x] == c) + (map[y][x+1] == c);

	if (sum >= 4) {
		map[y][x] = c;
		return c;
	}

	return 0;
}

void fill(struct cmv_data *data, int c) {
	struct reg_t *reg = data->cmax[c-1];
	int (*map)[W_2 + 1];
	int x, y;
	int sx;
	int sy;

	if (!reg)
		return;
	map = data->map;
	sx = reg->cx * reg->size;
	sy = reg->cy * reg->size;
	for (y=reg->y1+1; y<reg->y2; y++) {
		for (x = reg->x1+1; x < reg->x2; x++) {
			if (map[y][x] == 0) {
				int r;
				if ((r = cround (map, x, y, c))) {
					reg->size++;
					sx += x;
					sy += y;
				}
			}
		}
	}
	reg->cx = sx / reg->size;
	reg->cy = sy / reg->size;
}


void cmv_save(const struct cmv_data * data, const char *filename) {
	FILE* fp = fopen(filename, "w");
	int i;

	if (!fp) {
		mylogfd(2, "[Version|Cmv|cmv_save] fopen error %s", filename);
		return;
	}

	fprintf(fp, "%d\n", data->color_n);
	for (i=0; i<data->color_n; i++) {
		fprintf(fp, "%d %d %d %d %d %d\n", data->th[i][0], data->th[i][1],
				data->th[i][2], data->th[i][3], data->th[i][4], data->th[i][5]);
	}
	for (i=0; i<data->color_n; i++) {
		fprintf(fp, "%d ", data->remap[i]);
	}
	fprintf(fp, "\n\n");

	fprintf(fp, "%d\n", data->rgb_n);
	for (i=0; i<data->rgb_n; i++) {
		fprintf(fp, "%d %d %d %d\n", i, data->rgb[i][0],
				data->rgb[i][1], data->rgb[i][2]);
	}
	fprintf(fp, "\n");

	fprintf(fp, "%d\n", data->size_min);
	fprintf(fp, "%d %d\n", data->rle_max, data->reg_max);
	fclose(fp);
}


void cmv_reset_thresh(struct cmv_data *data, int c, int *v) {
	//	assert(c < data->color_n);
	int i;

	if (c >= data->color_n)
		return;
	if (v[0] > v[1] || v[2] > v[3] || v[4] > v[5])
		return;

	for (i=0; i<6; i++)
		data->th[c][i] = v[i];

	c = 1 << c;
	for (i=0; i<256; i++) data->thresh[0][i] &= ~c;
	for (i=0; i<256; i++) data->thresh[1][i] &= ~c;
	for (i=0; i<256; i++) data->thresh[2][i] &= ~c;
	set_bits(data->thresh[0], v[0], v[1], c);
	set_bits(data->thresh[1], v[2], v[3], c);
	set_bits(data->thresh[2], v[4], v[5], c);
}

/*
 * goal detection
 *
 * Parameters:
 * left,right: leftmost and rightmost edge at the bottom line.
 * isLeftArrived: if leftmost edge cannot be seen,then isLeftArrived=true.
 *                That means there're other parts of the goal beyond the left side of the camera.
 *  isRightArrived: the same as isLeftArrived.
 *
 *  Return value:
 *  If goal detected,return 1;otherwise return 0.
 * */
static inline int abs2(int x) {
	return x>0?x:-x;
	//return x;
}

int goal_detection(struct cmv_data *data, unsigned char* src,
	struct spot * left, struct spot * right, int* isLeftArrived, int* isRightArrived, struct local_parse_info * local_tmp,struct local_parse_info **data_t) {
	left->x = 0;right->x = 0;
    left->y = 0;right->y = 0;
    if (data->clist[COLOR_G] == 0)
		return 0;

	*isLeftArrived = 1;
	*isRightArrived = 1;

	int maxYValue[W];
	int tmpMaxYValue[W];
	int rawYValue[W];

	//record maximum of maxYValue[] and its cordinary x value.
	int maxmaxY=0, maxmaxXofY=0;

	memset(maxYValue,0,sizeof(maxYValue));

	int leftXEdge=2*data->cmax[COLOR_G]->x1+1,rightXEdge=2*data->cmax[COLOR_G]->x2-1;
	int maxYEdge=2*(data->cmax[COLOR_G]->y2);
	
	//mylogfd(logfd,"&&start max:%d\n",data);
	//calculate f(x)=max y
	int i = leftXEdge;
	for(;i<=rightXEdge&&i<W;i+=2) {
		int j = maxYEdge;
		for(;j>=0;j-=2) {
			//if it's goal's color, calculate max y
			int pos = (j/2) *(W_2 + 1) + i/2;
			int upos = pos > W_2 + 1 ? pos - W_2 - 1: pos;
			if( (data->map[pos] == (COLOR_G + 1))
			&&  (data->map[upos] == (COLOR_G + 1)) ) {
				maxYValue[i]=j;
				maxYValue[i+1]=j;
				break;
			}
		}
	}
	//fprintf(stderr,"find max over:%x\n",local_tmp->vparam);
	memcpy(tmpMaxYValue,maxYValue,sizeof(maxYValue));

	//calculate f(x)=avg(f(x-1),f(x),f(x+1)), so as to reduce noise
	for(i=rightXEdge-3;i>leftXEdge+2;i--){
		maxYValue[i] = (tmpMaxYValue[i-2]+tmpMaxYValue[i]+tmpMaxYValue[i+2])/3;
	}
	memcpy(tmpMaxYValue,maxYValue,sizeof(maxYValue));
	memcpy(rawYValue,maxYValue,sizeof(rawYValue));

	//calculate f'(x)
	for(i=rightXEdge;i>=0;i--){
		//calculate maximum of maxYValue[]
		if(tmpMaxYValue[i]>maxmaxY){
			maxmaxY=tmpMaxYValue[i];
			maxmaxXofY=i;
		}
		//if(ISDEBUG)printf("i: maxYValue[i] %d %d\n",i,maxYValue[i]);
		if(i>=W-2)
			maxYValue[i]=tmpMaxYValue[i]-tmpMaxYValue[i-2];
		else if(i<=1)
			maxYValue[i]=tmpMaxYValue[i+2]-tmpMaxYValue[i];
		else
			maxYValue[i]=(tmpMaxYValue[i+2]-tmpMaxYValue[i-2])/2;
	}
	memcpy(tmpMaxYValue,maxYValue,sizeof(maxYValue));
	//calculate f''(x)
	for(i=rightXEdge;i>=0;i--) {
		if(i>=W-2)
			maxYValue[i]=tmpMaxYValue[i]-tmpMaxYValue[i-2];
		else if(i<=1)
			maxYValue[i]=tmpMaxYValue[i+2]-tmpMaxYValue[i];
		else
			maxYValue[i]=(tmpMaxYValue[i+2]-tmpMaxYValue[i-2])/2;

                //printf("%d %d\n", i, maxYValue[i]);
	}
	data=data;
	//if(data_t!=0)
	//	fprintf(stderr,"&&start max'''':%d\n",*data_t);
	//fprintf(stderr,"find max'''' over:%x\n",local_tmp->vparam);
	int sumofzeropoint=0;
	int isrightmost=0;
	int isleftmost=0;

	int leftavg=0,rightavg=0;
	int tmpsum=0,tmpnum=0;

	//minimum of f''(x)
	int minfpp=0;

	for(i=rightXEdge;i>0;i--){
		if(maxYValue[i]<=-1){
			sumofzeropoint++;

			//calculate the average height of left and right point.
			//using maximum of 20 points.
			//left:
			tmpsum=0;
			tmpnum=0;
			int j=i-1;
			for(;j>=0 && tmpnum<=20;j-=2){
				tmpsum+=rawYValue[j];
				tmpnum++;
			}
			if(tmpnum==0)tmpnum=1;
			leftavg=tmpsum/tmpnum;

			//right:
			tmpsum=0;
			tmpnum=0;
			for(j=i+1;j<W && tmpnum<=20;j+=2){
				tmpsum+=rawYValue[j];
				tmpnum++;
			}
			if(tmpnum==0)tmpnum=1;
			rightavg=tmpsum/tmpnum;

			if(  (rightavg>leftavg)
				&&(abs2(i-2*data->cmax[COLOR_G]->x1)<35)){
				//if zero point meets the following conditions:
				//1)it's right point higher than left point
				//2)distance between itself and
				//left side of color range
				//is no too long
				//3)nearest to the maxmaxY
				//it's left edge point

				//first leftmost point meet?
				if(!isleftmost)
					isleftmost=1;
				//if not compare two distances to the maxmaxY
				//and f''(x) not reach the minimum
				else if((abs2(i-maxmaxXofY)
						>=abs2(left->x-maxmaxXofY))
					&& (maxYValue[i]>=minfpp)
				       )
					continue;
				left->x=i;
				left->y=rawYValue[i];
				*isLeftArrived = 0;
				if(maxYValue[i]<minfpp)
					minfpp=maxYValue[i];
			}
			else if(  ( rightavg<leftavg )
				  && (abs2(i-2*data->cmax[COLOR_G]->x2)<35)  ){
				//if zero point meets the following conditions:
				//it's right point lower than left point
				//and the distance between itself and
				//right side of color range
				//is no too long
				//it's right edge point
				if(!isrightmost)
					isrightmost = 1;
				else if((abs2(i-maxmaxXofY)
						>=abs2(right->x-maxmaxXofY))
					&&(maxYValue[i]>=minfpp)
				       )
					continue;
				right->x=i;
				right->y=rawYValue[i];
				*isRightArrived=0;
				if(maxYValue[i]<minfpp)
					minfpp=maxYValue[i];

			}
			else{
				//what's this?
			}
		}
	}
	//if(data_t!=0)
	//	fprintf(stderr,"&&start left right'''':%d %d\n",*data_t,local_tmp);
	data=data;
	//fprintf(stderr,"left right over:%x\n",local_tmp->vparam);
	if(*isLeftArrived){
		left->x=2*data->cmax[COLOR_G]->x1+3;
		int max=-1;
		for(i=leftXEdge;i<leftXEdge+25&&i<W;i++){
			if(max<rawYValue[i])
				max=rawYValue[i];
		}
		left->y=max;
		//left->y=rawYValue[left->x];
	}
	if(*isRightArrived){
		right->x=2*data->cmax[COLOR_G]->x2-3;
		int max=-1;
		int st=rightXEdge-25;
		if(st<=0)st=1;
		for(i=st;i<rightXEdge&&i<W;i++){
			if(max<rawYValue[i])
				max=rawYValue[i];
		}
		right->y=max;
		//right->y=rawYValue[right->x];
	}
	if(data_t!=0)
		mylogfd(logfd,"&&start over'''':%d %d\r",*data_t,local_tmp);
	mylogfd(logfd,"                        \r");
	return 1;
}


#define ANGLE_SAMELINE_OFFSET 15
#define ANGLE_VERY_CLOSE_OFFSET 4
#define RADIUS_DIFF_OFFSET 25
#define ANGLE_FIELD 45
#define DELTA PI / 2 / ANGLE_FIELD
#define RADIUS_LIMIT 320
#define LOCAL_MAX_THRESHOLD 35
#define DIS_LINE_THRESHOLD 20
int matrix[2*ANGLE_FIELD][RADIUS_LIMIT*2];
double aver_x[2*ANGLE_FIELD][RADIUS_LIMIT*2];
double aver_y[2*ANGLE_FIELD][RADIUS_LIMIT*2];

void accumulator(int x, int y,int weight) {
    double theta = - PI / 2;

    int i = 0;
    for (;i<2*ANGLE_FIELD;i++) {
        int r = (int)(cos(theta) * x + sin(theta) * y);
        if (r < RADIUS_LIMIT && r >= -RADIUS_LIMIT){
            matrix[i][r+RADIUS_LIMIT]+=weight;
            aver_x[i][r+RADIUS_LIMIT]+=x;
            aver_y[i][r+RADIUS_LIMIT]+=y;
		}
        theta += DELTA;
    }
}

int edge_detection(struct cmv_data *data, double * angle, int * radius, int * count)
{
	//TODO 边缘检测，检测多条边线
    memset(matrix,0,sizeof(matrix));
    memset(aver_x,0,sizeof(aver_x));
    memset(aver_y,0,sizeof(aver_y));

    int i = 2;
    for (;i<W_2-2;i++) {
        int j = 2;
        for(;j<H_2-2;j++) {
            int pos = j * (W_2 + 1) + i;
            //不是场地的颜色的话就不进行下面的处理
            //if (data->map[pos] != (COLOR_C + 1)&&data->map[pos]!=0)
            if (data->map[pos] != (COLOR_C + 1))
                continue;
            //int weight=(data->map[pos] == (COLOR_C + 1))?2:1;
            int weight;
            int tpos = (j + 1) * (W_2 + 1) + i;
            int ttpos = (j + 2) * (W_2 + 1) + i;
			if ((data->map[tpos] == COLOR_L + 1)
                || ((data->map[tpos] == (COLOR_C + 1) )
                    && ((data->map[ttpos] == COLOR_L + 1)))) {
				weight=(data->map[tpos] == (COLOR_L + 1))?2:1;
                accumulator(i,H_2-1-j,weight);
                continue;
            }

            //取上一行和上两行的点
            tpos = (j - 1) * (W_2 + 1) + i;
            ttpos = (j - 2) * (W_2 + 1) + i;
            if ((data->map[tpos] == COLOR_L + 1)
                || ((data->map[tpos] == (COLOR_C + 1) )
                    && ((data->map[ttpos] == COLOR_L + 1)))) {
				weight=(data->map[tpos] == (COLOR_L + 1))?2:1;
                accumulator(i,H_2-1-j,weight);
                continue;
            }
            tpos = j * (W_2 + 1) + i + 1;
            ttpos = j * (W_2 + 1) + i + 2;
            if ((data->map[tpos] == COLOR_L + 1)
                || ((data->map[tpos] == (COLOR_C + 1) )
                    && ((data->map[ttpos] == COLOR_L + 1)))) {
				weight=(data->map[tpos] == (COLOR_L + 1))?2:1;	
                accumulator(i,H_2-1-j,weight);
                continue;
            }
            tpos = j * (W_2 + 1) + i - 1;
            ttpos = j * (W_2 + 1) + i - 2;
            if ((data->map[tpos] == COLOR_L + 1)
                || ((data->map[tpos] == (COLOR_C + 1) )
                    && ((data->map[ttpos] == COLOR_L + 1)))) {
				weight=(data->map[tpos] == (COLOR_L + 1))?2:1;
                accumulator(i,H_2-1-j,weight);
                continue;
            }
        }
    }
	//fprintf(stderr,"end mark\n");
    int line_i[MAX_LINES];
    int line_j[MAX_LINES];
    int line_m[MAX_LINES];
    int line_i2[MAX_LINES];
    int line_j2[MAX_LINES];
    int line_m2[MAX_LINES];

    *count = 0;

	for (i=0;i<2*ANGLE_FIELD;i++) {
        int j = 0;
        for (;j<2*RADIUS_LIMIT;j++) {
			if(matrix[i][j]!=0){
				aver_x[i][j]/=matrix[i][j];
				aver_y[i][j]/=matrix[i][j];
			}
		}
	}
		//fprintf(stderr,"start find\n");
    for (i=0;i<2*ANGLE_FIELD;i++) {
        int j = 0;
        for (;j<2*RADIUS_LIMIT;j++) {
            if (matrix[i][j] < LOCAL_MAX_THRESHOLD)
                continue;
            if ((i>0 || matrix[i][j] > matrix[i-1][j] + LOCAL_MAX_THRESHOLD)
                && (i<2*ANGLE_FIELD-1 || matrix[i][j] > matrix[i+1][j] + LOCAL_MAX_THRESHOLD)
                && (j>0 || matrix[i][j] > matrix[i][j-1] + LOCAL_MAX_THRESHOLD)
                && (j<2*RADIUS_LIMIT-1 || matrix[i][j] > matrix[i][j+1] + LOCAL_MAX_THRESHOLD)) {
                int k = 0;
                int flag = 0;
				
                for (;k<*count;k++) {
                    if ((abs(line_i[k]-i) <= ANGLE_SAMELINE_OFFSET)
                         || (abs(line_i[k]-i) >= (2 * ANGLE_FIELD -  ANGLE_SAMELINE_OFFSET))){//可能是同一条线
						//可能是平行线
                        if ((abs(line_i[k]-i) <= ANGLE_VERY_CLOSE_OFFSET)
                         || (abs(line_i[k]-i) >= (2 * ANGLE_FIELD -  ANGLE_VERY_CLOSE_OFFSET))){	 
							if((abs(abs(line_j[k]-RADIUS_LIMIT)-abs(j-RADIUS_LIMIT)) >= RADIUS_DIFF_OFFSET)&&
							(hypot(aver_x[i][j]-aver_x[line_i[k]][line_j[k]],aver_y[i][j]-aver_y[line_i[k]][line_j[k]])>DIS_LINE_THRESHOLD)){
								//fprintf(stderr, "## an=%d, dis=%d, an2=%d, dis2=%d, count=%d\n",i,j,line_i[k],line_j[k],k);
								continue;
							}
						}
                        if (line_m[k] < matrix[i][j]) {
							//fprintf(stderr, "&& an=%d, dis=%d, an2=%d, dis2=%d, count=%d\n",i,j,line_i[k],line_j[k],k);
                            line_i[k] = i;
                            line_j[k] = j;
                            line_m[k] = matrix[i][j];
                        }
                        flag = 1;
                        break;
                    }
                }

                if (!flag) {
					//fprintf(stderr, "@@ an=%d, dis=%d\n",i,j);
                    line_i[*count] = i;
                    line_j[*count] = j;
                    line_m[*count] = matrix[i][j];
                    (*count)++;
                }
            }
        }
    }
    	//fprintf(stderr,"start adjust\n");
	int count_t=0;
    for(i=0;i<*count;i++){
		 int flag=0,k=0;
		 for (;k<count_t;k++) {
			if ((abs(line_i2[k]-line_i[i]) <= ANGLE_SAMELINE_OFFSET)
				 || (abs(line_i2[k]-line_i[i]) >= (2 * ANGLE_FIELD -  ANGLE_SAMELINE_OFFSET))){//可能是同一条线
				//可能是平行线
				if ((abs(line_i2[k]-line_i[i]) <= ANGLE_VERY_CLOSE_OFFSET)
				 || (abs(line_i2[k]-line_i[i]) >= (2 * ANGLE_FIELD -  ANGLE_VERY_CLOSE_OFFSET))){	 
					if((abs(abs(line_j2[k]-RADIUS_LIMIT)-abs(line_j[i]-RADIUS_LIMIT)) >= RADIUS_DIFF_OFFSET)&&
					(hypot(aver_x[line_i[i]][line_j[i]]-aver_x[line_i2[k]][line_j2[k]],aver_y[line_i[i]][line_j[i]]-aver_y[line_i2[k]][line_j2[k]])>DIS_LINE_THRESHOLD)){
						//fprintf(stderr, "## an=%d, dis=%d, an2=%d, dis2=%d, count=%d\n",i,j,line_i[k],line_j[k],k);
						continue;
					}
				}
				if (line_m2[k] < matrix[line_i[i]][line_j[i]]) {
					//fprintf(stderr, "&& an=%d, dis=%d, an2=%d, dis2=%d, count=%d\n",i,j,line_i[k],line_j[k],k);
					line_i2[k] = line_i[i];
					line_j2[k] = line_j[i];
					line_m2[k] = matrix[line_i[i]][line_j[i]];
				}
				flag = 1;
				break;
			}
		}

		if (!flag) {
			//fprintf(stderr, "@@ an=%d, dis=%d\n",i,j);
			line_i2[count_t] = line_i[i];
			line_j2[count_t] = line_j[i];
			line_m2[count_t] = matrix[line_i[i]][line_j[i]];
			count_t++;
		}
	}
	//fprintf(stderr, "XXXXXXXXXX\n");
	*count=count_t;
    for (i=0;i<*count;i++) {
        //printf("%d %d\n", line_i[i], line_j[i]);
        //fprintf(stderr, "an=%d, dis=%d, matix=%d, x=%f,  y=%f \n",line_i[i],line_j[i],matrix[line_i[i]][line_j[i]],aver_x[line_i[i]][line_j[i]],aver_y[line_i[i]][line_j[i]]);
        angle[i] = - PI /2 + line_i2[i] * DELTA;
        radius[i] = line_j2[i] - RADIUS_LIMIT;
    }
    return 0;
}

#define CIRCLE_CENTER_OFFSET 5
#define CENTER_POSITIONS (2*CIRCLE_CENTER_OFFSET+1)*(2*CIRCLE_CENTER_OFFSET+1)
#define MAXIMUM_RADIUS 40
#define BALL_LOCAL_MAX_THRESHOLD 5

int ball_matrix[CENTER_POSITIONS][MAXIMUM_RADIUS];
int centerX = -1;
int centerY = -1;
void ball_accumulator(int x, int y) {
    int i = -CIRCLE_CENTER_OFFSET;
    int j = -CIRCLE_CENTER_OFFSET;
    for (i=-CIRCLE_CENTER_OFFSET;i<=CIRCLE_CENTER_OFFSET;i++) {
        for (j = -CIRCLE_CENTER_OFFSET;j<=CIRCLE_CENTER_OFFSET;j++) {
            int pos = (j+CIRCLE_CENTER_OFFSET)*(2*CIRCLE_CENTER_OFFSET+1)+
            (i+CIRCLE_CENTER_OFFSET);
            int radius = (int)(hypot(centerX+i-x, centerY+j-y));
            if (radius < MAXIMUM_RADIUS){
                ball_matrix[pos][radius++]+=50;
			}
            while (radius < MAXIMUM_RADIUS){
                ball_matrix[pos][radius]+=50-radius;
                radius++;
			}
            //printf("%d %d %d\n", pos, radius, ball_matrix[pos][radius]);

        }
    }
}

int ball_outline_detection(struct cmv_data *data, int* x, int* y, int* radius) {
    if (!data->cmax[COLOR_B])
        return 0;

    int leftEdge = data->cmax[COLOR_B]->x1;
    int rightEdge = data->cmax[COLOR_B]->x2;
    int upEdge = data->cmax[COLOR_B]->y1;
    int downEdge = data->cmax[COLOR_B]->y2;
    //if (leftEdge <= 1 || rightEdge >= 158 || upEdge <= 1 || downEdge >= 118)
    //    return 0;

    memset(ball_matrix,0,sizeof(ball_matrix));

    centerX = (leftEdge + rightEdge) / 2;
    centerY = H_2 - 1 - (upEdge + downEdge) / 2;
    int i = leftEdge;
    int j = upEdge;
    for (i=leftEdge;i<=rightEdge;i++) {
        for (j=upEdge;j<=downEdge;j++) {
            int pos = j * (W_2 + 1) + i;
            if (data->map[pos] == (COLOR_B + 1)) {
                ball_accumulator(i,H_2-1-j);
                //printf("%d %d\n", i, H_2-1-j);
                break;
            }
        }
    }

    int max = 0;
    for (j=0;j<MAXIMUM_RADIUS;j++){
		for (i=0;i<CENTER_POSITIONS;i++) {
            //if (ball_matrix[i][j] > 5)
                //printf("%d %d %d\n", i, j, ball_matrix[i][j]);
            if (ball_matrix[i][j] > max) {
                max = ball_matrix[i][j];
                *x = 2 * (centerX + i % (2*CIRCLE_CENTER_OFFSET+1) - CIRCLE_CENTER_OFFSET) + 1;
                *y = 2 * (H_2 - 1 - (centerY + i / (2*CIRCLE_CENTER_OFFSET+1) - CIRCLE_CENTER_OFFSET)) - 1;
                *radius = 2 * j;
            }
        }
        //max+=2;
    }
    //printf("%d %d %d %d\n", *x, *y, *radius, max);
    return 1;
}

//
// name: use the translate matrix to calc postion(x,y) from the image point(u,v)
// @param	u,v: 图像中的点左标
//			x,y: 机器人坐标
// @return
void getxy(double u, double v, int *x, int *y, double param[])
{
	double temp;

	temp = param[6]*u + param[7]*v +1;
	*x = (param[0]*u + param[1]*v + param[2])/temp;
	*y = (param[3]*u + param[4]*v + param[5])/temp;
}

//
// name: 找球函数
// @param
// @return
int catchball(struct reg_t *reg) {
	int x1thresh = 60;
	int y1thresh = 40;
	int x2thresh = 155;
	return (reg->x1 > x1thresh && reg->y1 > y1thresh && reg->x2 > x2thresh);
}

//
// name: 未知
// @param
// @return
int left_right(int x, int y) {
	double b = -96.2069;
	double k = 2.3103;
	double a = ((double)y - b) / k;
	double dx = x - a;
	return (int)(dx);
	//this is in the netviewer
    //return (x - y / BALL_NEAR_K - BALL_NEAR_X);
}
int find_ball(int num, struct cmv_data * data_tmp,struct local_parse_info * info_tmp)
{
	struct reg_t *reg;
	reg = data_tmp->cmax[COLOR_B];
	info_tmp->is_ball_acc = 0;
	info_tmp->is_ball_seen=0;
	info_tmp->ball_acc_pos.x=-10000;
	info_tmp->ball_acc_pos.y=-10000;
	info_tmp->ball_pos.x=-10000;
	info_tmp->ball_pos.y=-10000;

	//fprintf(stderr,"[CMV] find ball\n");
	if (data_tmp->clist[COLOR_B]) {
		//fprintf(stderr,"[CMV] there is COLORB\n");
		int r = (reg->y2 - reg->y1)/2;
		int cx = (reg->x1 + reg->x2) / 2;
		if (reg->x1 <= 1)
			cx = reg->x2 - r;
		else if (reg->x2 >= 159)
			cx = reg->x1 + r;
		//fprintf(stderr,"[CMV] y2: %d\n",reg->y2);
		if (reg->y2 >= BOTTOM_LINE) {	// y2 reaches the bottom
			int rx2=0,ry2=0,bx2,by2;
			info_tmp->ball_pos.y = LOC_Y_VERY_NEAR; /* ball very near */
			info_tmp->is_ball_seen=1;
			int turn = left_right(reg->x1, reg->y1);

			if(reg->y1>10){
				info_tmp->is_ball_acc=1;
				bx2 = cx<<1;
				by2 = (reg->y1<<1);
				getxy(bx2, by2, &rx2, &ry2, info_tmp->vparam);
				info_tmp->ball_acc_pos.x=rx2*(ROBO_H-BALL_H)/ROBO_H;
				info_tmp->ball_acc_pos.y=ry2*(ROBO_H-BALL_H)/ROBO_H;
				if (info_tmp->ball_acc_pos.x < -6)
					info_tmp->ball_pos.x = LOC_LEFT;
				else if (info_tmp->ball_acc_pos.x > 6)
				//else if (turn > BALL_CENTER_OFFSET)
					info_tmp->ball_pos.x = LOC_RIGHT;
				else {
					info_tmp->ball_pos.x = 0;
					if(info_tmp->ball_acc_pos.x>-6&&info_tmp->ball_acc_pos.x<6 &&info_tmp->ball_acc_pos.y<16){
							//if (catchball(reg))
						info_tmp->ball_pos.y = LOC_CATCH_BALL;
					}
				}
				//fprintf(stderr,"2 x'=%d, y'=%d, x= %d, y=%d \n",rx2, ry2, info_tmp->ball_acc_pos.x,info_tmp->ball_acc_pos.y);
			}else{
				if (turn < -BALL_CENTER_OFFSET)
					info_tmp->ball_pos.x = LOC_LEFT;
				else if (turn > 40)
				//else if (turn > BALL_CENTER_OFFSET)
					info_tmp->ball_pos.x = LOC_RIGHT;
				else {
					info_tmp->ball_pos.x = 0;
					if (catchball(reg))
						info_tmp->ball_pos.y = LOC_CATCH_BALL;
				}
			}
		} else {
			//fprintf(stderr,"[CMV] above the bottom\n");
			int rx = 0, ry = 0;
			int rx2=0,ry2=0,bx2,by2;
			int bx=0, by=0, br=0;
			if (!ball_outline_detection(data_tmp, &bx, &by, &br)) {
				bx = cx<<1;
				by = (reg->y2<<1);
				getxy(bx, by, &rx, &ry, info_tmp->vparam);
				//fprintf(stderr,"@@@@@@@@--ball %d %d %d %d %d\n",num,bx,by,rx,ry);
			} else {
				//by = (by + br) / 2;
				//bx = bx / 2;
				by=by+br;
				getxy(bx, by, &rx, &ry, info_tmp->vparam);
				//fprintf(stderr,"@@@@@@@@ball %d %d %d %d %d\n",num,bx,by,rx,ry);
			}
			//fprintf(stderr,"&&&&&ball %d %d %d\n",num,rx,ry);
			if (cx < 0)
				info_tmp->ball_pos.x = LOC_LEFT;
			else if (cx > 159)
				info_tmp->ball_pos.x = LOC_RIGHT;
			else {
				info_tmp->is_ball_acc = 1;
				if(reg->y1>10){
					bx2 = bx;
					by2 = (reg->y1)<<1;
					getxy(bx2, by2, &rx2, &ry2, info_tmp->vparam);
					//fprintf(stderr,"2 x'=%d, y'=%d, ",rx2, ry2);
					//mylogfd(logfd,"@@@@@ball %d %d %d %d %d\r",num,bx2,by2,rx2,ry2);
					
					rx2=rx2*(ROBO_H-BALL_H)/ROBO_H;
					ry2=ry2*(ROBO_H-BALL_H)/ROBO_H;
					//fprintf(stderr,"&&&&&ball %d %d %d %d %d\n",num,rx,ry,rx2,ry2);
					if(ry2<1.3*ry&&ry2>0.7*ry){
						rx = (rx+rx2)/2;
						ry = (ry+ry2)/2;
					}
					//fprintf(stderr,"x= %d, y=%d \n",rx2,ry2);
				}
				info_tmp->ball_acc_pos.x = rx;
				info_tmp->ball_acc_pos.y = ry;
				if (rx < -6)
					info_tmp->ball_pos.x = LOC_LEFT;
				else if (rx > 6)
					info_tmp->ball_pos.x = LOC_RIGHT;
				else
					info_tmp->ball_pos.x = 0;
			}
			if (ry > DISTANCE_FAR ) {
				info_tmp->ball_pos.y = LOC_Y_INVALID;
			} else
				info_tmp->ball_pos.y = LOC_Y_NEAR;
			//fprintf(stderr,"1 x= %d, y=%d \n",info_tmp->ball_acc_pos.x,info_tmp->ball_acc_pos.y);
		}

		info_tmp->is_ball_seen = 1;
		return 0;
	}

	info_tmp->is_ball_seen = 0;
	return 1;
}

int find_robot(int num, struct cmv_data * data_tmp,struct local_parse_info * info_tmp) 
{
	struct reg_t *reg;
    reg = data_tmp->cmax[COLOR_R]; 	//ROBOT颜色的最大区域	
    info_tmp->is_robot_seen = 0;
    info_tmp->is_robot_acc= 0;
    if (data_tmp->clist[COLOR_R] && reg && reg->size > 120&&reg->y2>40) {
    	info_tmp->is_robot_seen = 1;
		int temp_cx, temp_cy;	//储存外包矩形底线中点的机器人坐标
		getxy(reg->x1 + reg->x2, reg->y2<<1, 
				&temp_cx, &temp_cy, info_tmp->vparam);
				
		if(temp_cx < -ROBOT_OFFSET)
			info_tmp->robot_pos.x = LOC_LEFT;
		else if(temp_cx > ROBOT_OFFSET)
			info_tmp->robot_pos.x = LOC_RIGHT;
		else
			info_tmp->robot_pos.x = 0;	    				
		if(reg->y2 > BOTTOM_LINE)
			info_tmp->robot_pos.y = LOC_Y_VERY_NEAR;
		else {
			info_tmp->robot_pos.y = LOC_Y_NEAR;
			info_tmp->is_robot_acc=1;
			info_tmp->robot_acc_pos.x=temp_cx;
			info_tmp->robot_acc_pos.y=temp_cy;
		}
		return 1;
	}
	return 0;
}

int find_door(int num,struct cmv_data * data_tmp, struct local_parse_info *local_tmp,unsigned char * yuv_buf)
{
	//TODO 门的下边沿检测，稳定
	/*
		以下四个变量是用图中的球门颜色块作为边界，直接计算的球门坐标（就是直接把这颜色区域作为球门，而不使用球门检测算法进行修正）
		机器人坐标
	*/
	int tldx, tldy, trdx, trdy;
		
	local_tmp->is_door_seen=1;
    local_tmp->is_door_right=0;
    local_tmp->is_door_left=0;
	//if(!data_tmp)
	//	return 0;
    struct reg_t *reg = data_tmp->cmax[COLOR_G];
    if (reg && data_tmp->clist[COLOR_G]) {

	//左右下角的点
		struct spot p_left, p_right;
		int leftmost, rightmost;	
		/*
			球门的检测算法，对球门颜色块进行处理，计算出两个角的点p_left,p_right。
			leftmost, rightmost是表明是否能看到左右门柱的，因为如果能看到右门柱，那么右下角的位置左移半个球门长，就可以得到球门中点。左门柱同理。
			只有能确定边缘确实是门柱，才可以根据这个边缘推测球门中心。
			
		*/
		
		goal_detection(data_tmp, yuv_buf, &p_left, &p_right, &leftmost, &rightmost,local_tmp,&local_tmp);
		//fprintf(stderr,"&&in find door:%d %d %d\n",data_tmp,local_tmp,yuv_buf);
		
		/*
			这个是一个修正，如果颜色块边缘非常靠近图像边缘，那么认为没有看到这一侧的门柱
		*/
		if (reg->x1 < 4)
			leftmost = 1;
		if (reg->x2 > 157)
			rightmost = 1;
				
			
			
		int lx, ly, rx, ry;
		//把图像坐标转换为机器人坐标
		getxy(p_left.x, p_left.y, &lx, &ly, local_tmp->vparam);
		getxy(p_right.x, p_right.y, &rx, &ry, local_tmp->vparam);

		//adjust in case the left menzhu is blocked
		//bug！！
		//getxy(reg->x1<<1, reg->y2<<1, &tldx, &tldy, local_tmp->vparam);
		getxy(reg->x1<<1, reg->y1<<1, &tldx, &tldy, local_tmp->vparam);
		getxy(reg->x2<<1, reg->y2<<1, &trdx, &trdy, local_tmp->vparam);
		
		/*
			以下算法主要是防止球门检测算法偏差太大，因为颜色块的边缘即使直接作为门柱偏差也不会太大
			如果计算的球门角与边缘差距较大，那么应该用边缘进行替代
		*/
		
		if (abs(tldx - lx) > 50 || abs(tldy - ly) > 90) {
			//mylogfd(1, "[Version|Pro|f_dor] Change the left axis\n");
			lx = tldx;
			ly = tldy;
		}
		if (abs(trdx - rx) > 50 || abs(trdy - ry) > 90) {
			//mylogfd(1, "[Version|Pro|f_dor] Change the right axis\n");
			rx = trdx;
			ry = trdy;
		}
		
		/*
			计算球门两个角之间的距离
		*/
		double r = hypot(rx-lx, ry-ly);
		/*
			下面计算球门中心坐标
		*/
		
		if (!leftmost) {//如果能看见左门柱，就用左门柱算中心的精确坐标
			local_tmp->door_acc_pos.x = lx + DOOR_LENGTH_2 * (rx - lx) / r;
			local_tmp->door_acc_pos.y = ly + DOOR_LENGTH_2 * (ry - ly) / r;
		} else if (!rightmost) {//看见右门柱，以此类推
			local_tmp->door_acc_pos.x = rx - DOOR_LENGTH_2 * (rx - lx) / r;
			local_tmp->door_acc_pos.y = ry - DOOR_LENGTH_2 * (ry - ly) / r;
		} else {//否则，都没看见，那么说明摄像头无法照下整个球门（此时离球门很近），用图像宽度，和一个角的y坐标，计算球门中心坐标
			getxy(160, reg->y2<<1, &local_tmp->door_acc_pos.x, &local_tmp->door_acc_pos.y, local_tmp->vparam);
			fprintf(stderr,"$$$$$$$DOOR: %d %d %d %d \n",160, reg->y2<<1, local_tmp->door_acc_pos.x, local_tmp->door_acc_pos.y);
		}

		if (!leftmost) {//看到左门柱
			local_tmp->is_door_left = 1;
			local_tmp->door_left.x = lx + DOOR_OFFSET * (rx - lx) / r;
			local_tmp->door_left.y = ly + DOOR_OFFSET * (ry - ly) / r;
		}
		if (!rightmost) {//看到右门柱
			local_tmp->is_door_right = 1;
			local_tmp->door_right.x = rx - DOOR_OFFSET * (rx - lx) / r;
			local_tmp->door_right.y = ry - DOOR_OFFSET * (ry - ly) / r;
		}
		//如果距离较远，那么给door_pos（大致坐标）一个LOC_Y_FAR标志，意思是比较远，精确坐标误差大
				if (local_tmp->door_acc_pos.y > DISTANCE_FAR) {
			local_tmp->door_pos.y = LOC_Y_FAR;

		}//否则 
		else{
			local_tmp->door_pos.y = DISTANCE_NEAR;                   
		}  

		/*
			下面给出了大致坐标的x标志，之所以使用大致坐标，是为了给策略模块一个说明（可以说明给出的精确坐标是否有意义，比如给一个“球门很远，但是在正中”，那么策略就可以直接往
			前走，不必管精确坐标。这样也可以加快处理速度
		*/
		if (local_tmp->door_acc_pos.x <-20)
			local_tmp->door_pos.x = LOC_LEFT;
		else if (local_tmp->door_acc_pos.x > 20)
			local_tmp->door_pos.x = LOC_RIGHT;
		else
			local_tmp->door_pos.x = 0;
    }
	else {
        /*door INVALIED*/
        local_tmp->is_door_seen=0;
    	local_tmp->is_door_left=0;
        local_tmp->is_door_right=0;
    }
    //fprintf(stderr,"&&out find door:%d %d %d\n",data_tmp,local_tmp,yuv_buf);
    return 1;
}

void judge_outside(struct cmv_data * data, struct local_parse_info *local_tmp) {
	int count_green_point = 0;
	int *m = data->map;
	int i, j;
	for (j=0; j<H_2; ++j) {
		for (i=0; i<W_2; ++i) {
			if(*(m++) ==COLOR_C+1) 
				count_green_point++;
		}
		m++;
	}
	local_tmp->green_sum=count_green_point;
	local_tmp->is_outside = (count_green_point >= 5500) ? 0 : 1;
}

void getEdgeDistance(struct spot left, struct spot right, int * dis, double * angle, struct local_parse_info * local_tmp)
{
    double lx = left.x + 0.375*(right.x-left.x);
    double ly = left.y + 0.375*(right.y-left.y);
    double rx = left.x + 0.625*(right.x-left.x);
    double ry = left.y + 0.625*(right.y-left.y);
    int rlx, rly, rrx, rry;
    getxy(lx, ly, &rlx, &rly, local_tmp->vparam);
    getxy(rx, ry, &rrx, &rry, local_tmp->vparam);
    double k = ((double)(rry-rly))/(rrx-rlx);
    *dis = (int)(rry - k*rrx);
    *angle = atan(k);
    //printf("y=%fx+%d;(%d,%d);(%d %d)\n", k, *dis, rlx, rly, rrx, rry);
    //printf("DIS: %d; AGL: %f\n", *dis, *angle);
}

//
// name: read in the translate matrix
// @param
// @return
void translate_init(struct local_parse_info * local_info)
{

	int i;
	FILE *fin = fopen(transfile, "r");

	if (fin == NULL) {
		mylogfd(2,"[Version|CMV|trans_init] Read translate.conf fail\n");
		perror(transfile);
		exit(-1);
	}
	//fprintf(stderr,"^^^0 ");
	for(i=0;i<8;i++) {
		if(fscanf(fin, "%lf", local_info[0].vparam+i) != 1) {
			mylogfd(2,"[Version|Pro|trans_init] Read uv2xy param error\n");
			exit(-1);
		}
		//fprintf(stderr," %lf ",local_info[0].vparam[i]);
	}
	//fprintf(stderr,"\n^^^1 ");
	for(i=0;i<8;i++) {
		if(fscanf(fin, "%lf", local_info[1].vparam+i) != 1) {
			mylogfd(2,"[Version|Pro|trans_init] Read uv2xy param error\n");
			exit(-1);
		}
		//fprintf(stderr," %lf ",local_info[1].vparam[i]);
	}
	//fprintf(stderr,"\n");
	for(i=0;i<2;i++){
		local_info[i].current_count=0;
		int j;
		local_info[i].is_door_seen=0;
		local_info[i].is_door_right=0;
		local_info[i].is_door_left;
		local_info[i].ball_radius=-1;
		local_info[i].is_ball_acc=0;//是否可以精确定位球
		local_info[i].is_ball_seen=0;//是否可以看见球
		local_info[i].is_robot_acc=0;
		local_info[i].is_robot_seen=0;
	}
	fclose(fin);
}
