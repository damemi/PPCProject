# PPCProject
Final project for Parallel Programming and Computing at RPI

This program takes as input an image encoded into a text file, and outputs an encoded sorted version of that input image. There is a helper program, called "img_to_txt" which is capable of both converting an image file to a text encoding and vice versa.

It should be noted that the helper program requires the OpenCV library to run, but the main program uses only standard C++ libraries and MPI. To compile both, simply run make.

The usage for the img_to_txt program is as follows:
    ./img_to_txt.o <input file> <output file> <encode/decode (0/1)>
Where <input file> is the file to be converted, and <output file> is converted file to be created. The <encode/decode> parameter can be either 0 or 1, where 0 encodes an image file to text and 1 decodes a text file to an image.

The usage for the main sorting program is as follows:
    mpirun -n <processes> ./project.o <input file> <output file>
Where <processes> is the number of MPI tasks, <input file> is the file to be sorted and <output file> is the file to be created.

It should be noted that we had the most success using images whose dimensions were a multiple of 2. As an example, we've included an image "sample.png" which is 256x256.