CFLAGS = -Wall -I../include -O2
LIBS = -ljpeg ./librobot.a -lpthread -lm
PROGRAMS = calib jpegdec.o process.o

all: $(PROGRAMS)

calib: calib.cpp jpegdec.o process.o
	g++ `pkg-config opencv --libs --cflags opencv` $(CFLAGS) $^ -o $@ $(LIBS)

jpegdec.o: jpegdec.c
	gcc -c $(CFLAGS) $^ -o $@

process.o: process.c
	gcc -c $(CFLAGS) $^ -o $@

clean: 
	rm -f $(PROGRAMS)
