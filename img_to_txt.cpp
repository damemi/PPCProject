/***************************************************************************/
/* Includes ****************************************************************/
/***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cv.h>
#include <highgui.h>
#include <iostream>
#include <fstream>

using namespace cv;

Mat img;

void encode(char*, char*);
void decode(char*, char*);

int main(int argc, char *argv[])
{
	if(argc < 4)
	{
		printf("Usage: <image-file-name> <text-file-name> < 0 for encode / 1 for decode>\n");
		exit(0);
	}

	if (atoi(argv[3]) == 0) // Encode (img to text)
	{
		encode(argv[1], argv[2]);	
	}
	else if (atoi(argv[3]) == 1) // Decode (text to img)
	{
		decode(argv[1], argv[2]);
	}
	return 0;
}

void encode(char* img_file_name, char* txt_file_name)
{
	img = imread(img_file_name, CV_LOAD_IMAGE_COLOR);
	std::ofstream myfile;
	myfile.open(txt_file_name);
	myfile << img.rows << " " << img.cols << "\n";
	for(MatIterator_<Vec3b> it = img.begin<Vec3b>(), end = img.end<Vec3b>(); it != end; ++it)
	{
		/* std::cout << (unsigned int)(*it)[0] << std::endl; */
		myfile << (unsigned int)(*it)[0] << " " << (unsigned int)(*it)[1] << " " << (unsigned int)(*it)[2] << " ";
	}
	myfile.close();
	return;
}

void decode(char* img_file_name, char* txt_file_name)
{
	unsigned int width, height;
	FILE* myfile = fopen(txt_file_name, "r");
	fscanf(myfile, "%u %u\n", &width, &height);
	/* Mat img(width, height, CV_8UC3, Scalar(0,255,0)); */
	img.create(width, height, CV_8UC3);
	MatIterator_<Vec3b> it = img.begin<Vec3b>();
	while (it != img.end<Vec3b>())
	{
		unsigned int a, b, c;
		fscanf(myfile, "%u %u %u ", &a, &b, &c);
		/* std::cout << "a = " << a << " b = " << b << " c = " << c << std::endl; */
		(*it)[0] = a;
		(*it)[1] = b;
		(*it)[2] = c;
		/* std::cout << "a = " << (unsigned int)(*it)[0] << " b = " << (unsigned int)(*it)[1] << " c = " << (unsigned int)(*it)[2] << std::endl; */
		it++;
	}
	imwrite(img_file_name, img);
}
