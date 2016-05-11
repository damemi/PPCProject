all:
	mpic++ project.cpp -o project.out
	g++ -Wall -I./include/ img_to_txt.cpp -o img_to_txt.o `pkg-config --cflags --libs opencv` -lopencv_core -lopencv_highgui -g
