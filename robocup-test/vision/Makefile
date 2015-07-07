PROG=videocap
CFLAGS=-O3 -s -Wall -I ../include -L..
LIB = -ljpeg -lm

videocap: v4l2cap.c jpegdec.c cap2_net.c process.c utils.c ../my_utils.c ../librobot.a
	gcc $(CFLAGS) $(LIB) -o $@ $^ -lrobot

clean:
	rm -f $(PROG)
