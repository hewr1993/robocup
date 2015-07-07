CFLAGS=-Wall -g  -O2 -I ../include

OBJECTS=cmv.o cvt.o dec.o
PROG=librobot.a

all: $(PROG)

$(PROG): $(OBJECTS)
	$(AR) rcs $@ $^
	rm -f *.o
	cp librobot.a ..

clean:
	rm $(PROG)
