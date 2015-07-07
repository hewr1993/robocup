all:robocup

robocup:main.o capture.o robomap.o astar.o
	g++ $^ -lopencv_core -lopencv_highgui -lopencv_features2d -lopencv_nonfree -lopencv_ml -o $@

.cpp.o:
	g++ $^ -c -o $@

clean:
	rm -f *.o robocup
