all:
	g++ -Wall -I/usr/include/opencv project.cpp -o project.o `pkg-config --cflags --libs opencv` -lopencv_core -lopencv_highgui
