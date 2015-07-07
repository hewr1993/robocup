#ifndef JPEGDEC_H
#define JPEGDEC_H

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#include "jpeglib.h"

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in
 * the second part of the example.
 */

#include <setjmp.h>


#include <stdio.h>


//image information
//including width and height
struct img_info {
    int width;
    int height;
};


/*
 * New jpeg_decompress_struct
 * creat buffer and decompress struct
 * @parameters:
 * 	width:	image width. This variable defines the one-line buffer size,
 * 		so it is not necessarily equal to actual image width.
 * 		You can set it as large as you can(e.g., 10000), but it should
 * 		not be smaller than actual image width.
 */
struct jpeg_decompress_struct* newDecompressor(int width);

/*
 * Delete jpeg_decompress_struct
 * @parameters:
 * 	cinfo:	jpeg_decompress_struct
 */
void deleteDecompressor(struct jpeg_decompress_struct* cinfo);

/*
 * Decode jpeg image
 * @parameters:
 * 	cinfo:		decode header
 * 	jpgbuffer:	source jpeg buffer
 * 	buffersize:	buffer size
 * 	destbuffer:	destination buffer
 * 	destbuffersize:	buffer size
 * 	format:		output format, 0-RGB(default)
 * 				       1-YUV
 * @return values:
 * 	1		if success
 */
int read_JPEG_buffer(struct jpeg_decompress_struct *cinfo, //header
                     unsigned char*jpgbuffer, //src buffer, stores mjpeg
                     long buffersize, //src buffer size
                     unsigned char* destbuffer, //destination buffer, rgb
                     long destbuffersize,//buffer size
                     struct img_info* image_info ,//buffer size
                     int format);


//this function is almost the same as read_JPEG_buffer, except that it uses
//file as an input source.
int read_JPEG_buffer_from_file(struct jpeg_decompress_struct *cinfo, //header
                               FILE* file, //file handle. It has been opened.
                               unsigned char* destbuffer, //destination buffer, rgb
                               long destbuffersize,//buffer size
                               struct img_info* image_info);//img info


//Return 1 if an fatal error occurs;
//If it is true, decoding process can not continue any more;
//Aplication should destroy jpeg_decompress_struct function,
//and re-build the whole jpeg_decompress_struct.
int isfatalerror();

#endif
