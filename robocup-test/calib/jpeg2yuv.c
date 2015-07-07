#include "jpegdec.h"

#define MAX_NET_WIDTH	320
#define MAX_NET_HEIGHT	240

 (double x) {
        int r = x;      /* round to nearest */

        if (r < 0)         return 0;
        else if (r > 255)  return 255;
        else               return r;
}
void rgb2yuv(unsigned char r, unsigned char g, unsigned char b, unsigned char* Y1, unsigned char* Cb, unsigned char* Cr){    *Y1 = clamp((( ( 66 * r + 129 * g + 25 * b + 128) >> 8) + 16));
    *Cb = clamp(( ( -38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
    *Cr = clamp(( ( 112 * r - 94 * g - 18 * b + 128) >> 8) + 128 );
}
int jpeg2yuv(unsigned char* tmpbuf, int buffersize, unsigned char* yuvbuf, int* size) {
	struct jpeg_decompress_struct* jpeg_decompressor=newDecompressor(MAX_NET_WIDTH);
	long rgbbuffersize;
	unsigned char rgbbuffer[MAX_NET_WIDTH*MAX_NET_HEIGHT*3];
	if(read_JPEG_buffer(jpeg_decompressor,
				tmpbuf,
				buffersize,
				rgbbuffer,
				rgbbuffersize,
				NULL,
				0) !=1){
					fprintf(stderr,"\nerror while decoding jpeg files.\n");
					
					//if fatal error occurs,
					if(isfatalerror()){
						fprintf(stderr,"\nwarning: fatal error occur. reconstructing decompress process.\n");
						//first delete old decompressor
						deleteDecompressor(jpeg_decompressor);
						//next create new decompressor
						jpeg_decompressor=newDecompressor(MAX_NET_WIDTH);
					}
					//exit(-1);
					return -1;
			}
	
	unsigned char* yp=yuvbuf;
	unsigned char* up=yp+W*H;
	unsigned char* vp=up+W*H/4;
	int h,w;
	//now convert from rgb to YUV420P
	for(h=0;h<H;h++){
		for(w=0;w<W;w++){
			//int y,cb,cr;
			unsigned char* rgbstart=rgbbuffer+(h*W+w)*3;
			rgb2yuv(*(rgbstart+0),//r
				*(rgbstart+1),//g
				*(rgbstart+2),//b
				yp,  //y
				up,  //u
				vp); //v
			yp++;
			if(h%2==1 && w%2==1){
				up++;
				vp++;
			}		
		}
	}
	*size = W*H*3/2;
	return 0;
}

