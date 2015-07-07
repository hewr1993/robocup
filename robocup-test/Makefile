CC=gcc
CFLAGS = -Wall -g  -I ./include -O2
LIBS = -lpthread -lm -static  
#-lreadline -ltermcap
PROGRAMS = robocup lib videocap motor_test net2_show
NAME = robocup-src

all: $(PROGRAMS)
motor_test:motor_test.c motor_control.c kbctrl.c my_utils.c
	gcc $(CFLAGS) motor_test.c motor_control.c kbctrl.c my_utils.c -o motor_test $(LIBS)

robocup: main.c handlers.c sched.c task.c motor_control.c my_utils.c kbctrl.c
	gcc $(CFLAGS) $^ -o $@ $(LIBS)
net2_show:lib
	$(MAKE) -C net_viewer
videocap:lib
	$(MAKE) -C vision
	cp vision/videocap .
lib: 
	$(MAKE) -C librobot

clean:
	rm -f robocup cscope.out
	rm -f motor_test
	rm -f videocap
	rm -f vision/videocap
	rm -f net_viewer/net2_show
	rm -f net_viewer/*.o
	rm -f *.o
	rm -f librobot/librobot.a
	rm -f *~
	rm -f librobot.a

handin:
	rm -f *.tar.gz
	rm -rf /tmp/${NAME}
	make clean
	mkdir /tmp/${NAME}
	cp -rf * /tmp/${NAME}
	cd /tmp && tar zcf ${NAME}-${ID}.tar.gz ${NAME}
	mv /tmp/${NAME}-${ID}.tar.gz .
	rm -rf /tmp/${NAME}
