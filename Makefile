all:
	gcc -Wall -I/usr/include/opencv project.c -o project.out `pkg-config --cflags --libs opencv` -lopencv_core -lopencv_highgui