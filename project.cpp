/***************************************************************************/
/* Includes ****************************************************************/
/***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cv.h>
#include <highgui.h>

/***************************************************************************/
/* Global Vars *************************************************************/
/***************************************************************************/

// TODO: Matrix rows
// TODO: Class for pixels
unsigned int **g_matrix = NULL;
unsigned int **g_sorted_matrix = NULL;

unsigned int height = 0;
unsigned int width = 0;
unsigned int step = 0;
unsigned int channels = 0;

unsigned int g_start_row = 0;
unsigned int g_end_row = 0;

unsigned int g_output_type = 1;

/***************************************************************************/
/* Function Decs ***********************************************************/
/***************************************************************************/

unsigned int ***allocate_matrix(unsigned int, unsigned int, unsigned int); // Allocate space for the matrix
unsigned int z_generator(unsigned int, unsigned int, unsigned int); // A helper function to calculate the z-curve value given RGB values
void init_matrix(int); // Take data from the image and store it to our rows

/***************************************************************************/
/* Function: Main **********************************************************/
/***************************************************************************/

int main(int argc, char *argv[])
{
  IplImage* img = 0; 
  uchar *data;
  int i,j,k;

  if(argc<2){
    printf("Usage: main <image-file-name>\n\7");
    exit(0);
  }

  // load an image  
  img=cvLoadImage(argv[1],1);
  if(!img){
    printf("Could not load image file: %s\n",argv[1]);
    exit(0);
  }

  // get the image data
  height    = img->height;
  width     = img->width;
  step      = img->widthStep;
  channels  = img->nChannels;
  if (channels != 3)
  {
    printf("Not an RGB image\n");
    exit(0);
  }
  data      = (uchar *)img->imageData;
  printf("Processing a %dx%d image with %d channels\n",height,width,channels); 

  // create a window
  /* cvNamedWindow("mainWin", CV_WINDOW_AUTOSIZE); */ 
  /* cvMoveWindow("mainWin", 100, 100); */

  // 

  // invert the image
  for(i=0;i<height;i++)
  {
    for(j=0;j<width;j++)
    {
      for(k=0;k<channels;k++)
      {
	data[i*step+j*channels+k]=255-data[i*step+j*channels+k];
      }
    }
  }

  std::cout << z_generator(0,255,255) << std::endl;

  // show the image
  /* cvShowImage("mainWin", img ); */

  // wait for a key
  /* cvWaitKey(0); */

  // release the image
  /* cvReleaseImage(&img ); */

  return 0;
}


/***************************************************************************/
/* Other Functions *********************************************************/
/***************************************************************************/

/***************************************************************************/
/* Function: Allocate Matrix ***********************************************/
/***************************************************************************/

unsigned int ***allocate_matrix(unsigned int height, unsigned int width, unsigned int channels)
{
    // NOTE TO FUTURE TRAVELLERS: 
    // The matrix is an array of arrays 
    // It shouldn't matter if it's treated as rows of columns or columns of rows, as long as usage is consistent. 
    // For consistency: the matrix is an array of rows. 
    // That means to get the second item from the left, in the third row down, you use g_matrix[2][1] 
    // The elements themselves are 4 element arrays: [R, G, B, Z]
    // tl;dr: g_matrix[row of desired element][column of desired element]

    // Heavily based off of code found here:
    // http://stackoverflow.com/questions/9736554/how-to-use-mpi-derived-data-type-for-3d-array
    
    unsigned int *data = new unsigned int [height*width*channels];
    unsigned int ***array = new unsigned int **[height];
    for (unsigned int i=0; i<height; i++)
    {
      array[i] = new unsigned int *[width];
      for (unsigned int j=0; j<width; j++)
      {
	array[i][j] = &(data[(i*width+j)*channels]);
      }
    }
    return array;
}

/***************************************************************************/
/* Function: Z Generator ***************************************************/
/***************************************************************************/

// Credit: http://graphics.stanford.edu/~seander/bithacks.html

unsigned int z_generator(unsigned int R, unsigned int G, unsigned int B)
{
  unsigned int W = 0;
  unsigned int X = 0;
  unsigned int Y = 0;
  W |= (R << 16) & 0x800000;
  W |= (R << 14) & 0x100000;
  W |= (R << 12) & 0x020000;
  W |= (R << 10) & 0x004000;
  W |= (R << 8) & 0x000800;
  W |= (R << 6) & 0x000100;
  W |= (R << 4) & 0x000020;
  W |= (R << 2) & 0x000004;
  X |= (G << 16) & 0x800000;
  X |= (G << 14) & 0x100000;
  X |= (G << 12) & 0x020000;
  X |= (G << 10) & 0x004000;
  X |= (G << 8) & 0x000800;
  X |= (G << 6) & 0x000100;
  X |= (G << 4) & 0x000020;
  X |= (G << 2) & 0x000004;
  Y |= (B << 16) & 0x800000;
  Y |= (B << 14) & 0x100000;
  Y |= (B << 12) & 0x020000;
  Y |= (B << 10) & 0x004000;
  Y |= (B << 8) & 0x000800;
  Y |= (B << 6) & 0x000100;
  Y |= (B << 4) & 0x000020;
  Y |= (B << 2) & 0x000004;
  return (W | (X >> 1) | (Y >> 2));
}


/***************************************************************************/
/* Function: Init Matrix ***************************************************/
/***************************************************************************/

void init_matrix() //int myrank, int commsize)
{

}
