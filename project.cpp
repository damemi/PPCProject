/***************************************************************************/
/* Includes ****************************************************************/
/***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cv.h>
#include <highgui.h>
#include <mpi.h>

// Because I'm sick of typing cv::
using namespace cv;

/***************************************************************************/
/* Global Vars *************************************************************/
/***************************************************************************/

// TODO: Matrix rows
// TODO: Class for pixels
unsigned int ***g_matrix = NULL;
unsigned int ***g_sorted_matrix = NULL;

unsigned int height = 0;
unsigned int width = 0;

unsigned int g_start_row = 0;
unsigned int g_end_row = 0;

unsigned int g_numrows=0;
int g_numworkers=0;

unsigned int g_output_type = 1;

Mat img;

/***************************************************************************/
/* Function Decs ***********************************************************/
/***************************************************************************/

unsigned int ***allocate_matrix(unsigned int, unsigned int, unsigned int); // Allocate space for the matrix
unsigned int z_generator(unsigned int, unsigned int, unsigned int); // A helper function to calculate the z-curve value given RGB values
void init_matrix(Mat&); // Take data from the image and store it to our rows
void quicksort_serial(unsigned int, unsigned int);
Mat& save_img(Mat&); // Store the data from the matrix back into the image

/***************************************************************************/
/* Function: Main **********************************************************/
/***************************************************************************/

int main(int argc, char *argv[]) 
{

  int mpi_myrank;
  int mpi_commsize;
  double starttime, endtime;

  if(argc<3){
    printf("Usage: main <image-file-name> <output-file-name>\n\7");
    exit(0);
  }

  // load an image  
  img = imread(argv[1], CV_LOAD_IMAGE_COLOR);
  if(!img.data){
    printf("Could not load image file: %s\n",argv[1]);
    exit(0);
  }

  MPI_Init( &argc, &argv);
  MPI_Comm_size( MPI_COMM_WORLD, &mpi_commsize);
  MPI_Comm_rank( MPI_COMM_WORLD, &mpi_myrank);
  starttime = MPI_Wtime();
  
  g_numworkers = mpi_commsize;

  // get the image data
  height    = img.rows;
  width     = img.cols;

  // Calculate number of rows for each rank
  int averow = height/g_numworkers;
  int extra = height%g_numworkers;
  g_numrows = (mpi_myrank < extra) ? averow+1 : averow;  

  MPI_Barrier( MPI_COMM_WORLD );

  g_matrix = allocate_matrix(height, width, 4);
  init_matrix(img);

  //unsigned int i = g_numrows * width * mpi_myrank;
  //unsigned int j = i + (g_numrows - 1)*width;  
  //quicksort_serial(0, height*width - 1);
  quicksort_serial(g_numrows * width * mpi_myrank,
		   (g_numrows * width * mpi_myrank) + (g_numrows-1) * width);
  
  img = save_img(img);
  if(mpi_myrank == 0)
    imwrite(argv[2], img);

  MPI_Barrier( MPI_COMM_WORLD );

  endtime = MPI_Wtime();
  MPI_Finalize();

  if(mpi_myrank == 0) {
    printf("That took %f seconds\n",endtime-starttime);
  }


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

void init_matrix(Mat& I)
{
  unsigned int curr_x = 0;
  unsigned int curr_y = 0;
  unsigned int i = 1;
  unsigned int r, g, b, z;
  // accept only char type matrices
  CV_Assert(I.depth() == CV_8U);

  MatIterator_<Vec3b> it, end;
  for( it = I.begin<Vec3b>(), end = I.end<Vec3b>(); it != end; ++it, i++)
  {
    r = (*it)[0];
    g = (*it)[1];
    b = (*it)[2];
    z = z_generator(r, g, b);
    curr_y = (i - 1) / width;
    curr_x = (i - 1) % width;
    g_matrix[curr_y][curr_x][0] = r;
    g_matrix[curr_y][curr_x][1] = g;
    g_matrix[curr_y][curr_x][2] = b;
    g_matrix[curr_y][curr_x][3] = z;

    /* (*it)[0] = table[(*it)[0]]; */
    /* (*it)[1] = table[(*it)[1]]; */
    /* (*it)[2] = table[(*it)[2]]; */
  }
}

/***************************************************************************/
/* Function: Quicksort - Serial ********************************************/
/***************************************************************************/

// This function heavily based on code from:
// http://www.algolist.net/Algorithms/Sorting/Quicksort
void quicksort_serial(unsigned int left_bound, unsigned int right_bound)
{
  // left = 0, right = height*width
  
  unsigned int i = left_bound, j = right_bound;
  
  unsigned int tmp_r, tmp_g, tmp_b, tmp_z;
  unsigned int middle = (left_bound + right_bound) / 2;
  unsigned int pivot = g_matrix[middle / width][middle % width][3];

  /* partition */

  while (i < j)
  {
    while (g_matrix[i / width][i % width][3] < pivot)
    {
      i++;
    }
    while (g_matrix[j / width][j % width][3] > pivot)
    {
      j--;
    }
    if (i <= j)
    {
      tmp_r = g_matrix[i / width][i % width][0];
      tmp_g = g_matrix[i / width][i % width][1];
      tmp_b = g_matrix[i / width][i % width][2];
      tmp_z = g_matrix[i / width][i % width][3];
      g_matrix[i / width][i % width][0] = g_matrix[j / width][j % width][0];
      g_matrix[i / width][i % width][1] = g_matrix[j / width][j % width][1];
      g_matrix[i / width][i % width][2] = g_matrix[j / width][j % width][2];
      g_matrix[i / width][i % width][3] = g_matrix[j / width][j % width][3];
      g_matrix[j / width][j % width][0] = tmp_r;
      g_matrix[j / width][j % width][1] = tmp_g;
      g_matrix[j / width][j % width][2] = tmp_b;
      g_matrix[j / width][j % width][3] = tmp_z;
      i++;
      j--;
    }
  };

  /* recursion */

  if (left_bound < j)
  {
    quicksort_serial(left_bound, j);
  }
  if (i < right_bound)
  {
    quicksort_serial(i, right_bound);
  }
}

/***************************************************************************/
/* Function: Save Image ****************************************************/
/***************************************************************************/

Mat& save_img(Mat& I)
{
  unsigned int curr_x = 0;
  unsigned int curr_y = 0;
  unsigned int i = 1;
  // accept only char type matrices
  CV_Assert(I.depth() == CV_8U);

  MatIterator_<Vec3b> it, end;
  for( it = I.begin<Vec3b>(), end = I.end<Vec3b>(); it != end; ++it, i++)
  {
    curr_y = (i - 1) / width;
    curr_x = (i - 1) % width;

    (*it)[0] = g_matrix[curr_y][curr_x][0];
    (*it)[1] = g_matrix[curr_y][curr_x][1];
    (*it)[2] = g_matrix[curr_y][curr_x][2];
  }

  return I;
}
