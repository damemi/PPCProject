all:
	mpic++ -Wall -I./include/ project.cpp -o project.o `pkg-config --cflags --libs opencv` -lopencv_core -lopencv_highgui -g
