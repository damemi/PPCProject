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
int g_mpi_myrank;

Mat img;

/***************************************************************************/
/* Function Decs ***********************************************************/
/***************************************************************************/

unsigned int ***allocate_matrix(unsigned int, unsigned int, unsigned int); // Allocate space for the matrix
unsigned int z_generator(unsigned int, unsigned int, unsigned int); // A helper function to calculate the z-curve value given RGB values
void init_matrix(Mat&,int); // Take data from the image and store it to our rows
void quicksort_serial(unsigned int, unsigned int);
Mat& save_img(Mat&,int); // Store the data from the matrix back into the image

void swap(int, int);
int partition(int, int);
void quicksort(int, int);

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
  g_mpi_myrank = mpi_myrank;
  // get the image data
  height    = img.rows;
  width     = img.cols;

  // Calculate number of rows for each rank
  int averow = height/g_numworkers;
  int extra = height%g_numworkers;
  g_numrows = (mpi_myrank < extra) ? averow+1 : averow;  

  MPI_Barrier( MPI_COMM_WORLD );

  g_matrix = allocate_matrix(height, width, 4);
  //g_matrix = allocate_matrix(g_numrows, width, 4);
  init_matrix(img,mpi_myrank);

  //quicksort(0, height*width - 1);
  //quicksort(0,g_numrows*width-1);


  //////
  // Start parallel stuff
  /////
  int i;
  int length = g_numrows*width*4; // length of contiguous data array
  MPI_Status status;
  // Send all of the data to processor 0
  if (mpi_myrank == 0) {
    for (i=1; i<g_numworkers; i++)
      MPI_Recv(&(g_matrix[0][0][0])+i*length, length, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  }
  else {
    MPI_Send(&(g_matrix[0][0][0]), g_numrows*width*4, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }

  int s;
  //int localDataSize = g_numrows*width;
  int localDataSize = height*width;
  int pivot;
  for (s=g_numworkers; s > 1; s /= 2) {
    if (mpi_myrank % s == 0) {
      pivot = partition(0, localDataSize-1);

      // Send everything after the pivot to processor rank + s/2 and keep up to the pivot
      MPI_Send(&(g_matrix[0][0][0])+pivot*4, localDataSize*4-pivot*4, MPI_INT, mpi_myrank + s/2, 0, MPI_COMM_WORLD);
      localDataSize = pivot;
    }
    else if (mpi_myrank % s == s/2) {
      // Get data from processor rank - s/2
      MPI_Recv(&(g_matrix[0][0][0]), localDataSize*4, MPI_INT, mpi_myrank - s/2, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

      // How much data did we really get?
      MPI_Get_count(&status, MPI_INT, &localDataSize);
      localDataSize /= 4;
    }
  }

  //quicksort(0, g_numrows*width-1);
  quicksort(0, localDataSize-1);

  if(mpi_myrank==0) {
    unsigned int p,q,z=0;
    for(p=0;p<localDataSize/width;p++) {
      for(q=0;q<localDataSize%width;q++) {
	printf("%d %d\n",g_matrix[p][q][3], z);
	z++;
      }
    }
  }
  //////////
  // End parallel stuff
  //////////
  
  img = save_img(img,mpi_myrank);


  std::ostringstream stream;
  stream << mpi_myrank << argv[2];
  std::string outputName = stream.str();
  //if(mpi_myrank == 0)
    //imwrite(argv[2], img);
  imwrite(outputName,img);

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

void swap(int i, int j) {
  unsigned int tmp_r, tmp_g, tmp_b, tmp_z;
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
}

int partition(int start, int end) {
  if (start >= end) return 0;
  int middle = (start + end) / 2;
  unsigned int pivotValue = g_matrix[middle / width][middle % width][3];
  //unsigned int pivotValue = g_matrix[start / width][start % width][3];
  int low = start;
  int high = end;

  while (low <= high) {
    while (g_matrix[low/width][low%width][3] <= pivotValue && low < end) {
      low++;
    }
    while (g_matrix[high/width][high%width][3] > pivotValue && high > start) {
      high--;
    }
    if (low <= high) { 
      swap(low, high);
      low++;
      high--;
    }
  }

  swap(start, high);

  return high;
}

void quicksort(int start, int end) {
  if  (end-start+1 < 2) return;
  int pivot = partition(start, end);
  if(start < pivot)
    quicksort(start, pivot);
  if(pivot+1 < end)
    quicksort(pivot+1, end);
}



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

void init_matrix(Mat& I, int mpi_myrank)
{
  unsigned int curr_x = 0;
  unsigned int curr_y = 0;
  unsigned int i = 1;
  unsigned int r, g, b, z;
  // accept only char type matrices
  CV_Assert(I.depth() == CV_8U);

  int j, k;

  int startX = 0;
  int startY = g_numrows * mpi_myrank;
  int endX = I.cols;
  int endY = startY + g_numrows;

  //MatIterator_<Vec3b> it, end;
  //for( it = I.begin<Vec3b>(), end = I.end<Vec3b>(); it != end; ++it, i++)
  for(j=startY; j<endY; j++) {
    for(k=startX; k<endX; k++)
      {
	Vec3b it = I.at<Vec3b>(j,k);
	r = (it)[0];
	g = (it)[1];
	b = (it)[2];
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
	i++;
      }
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
  //871 870 869 871
    }
  }

  /* recursion */
  if (left_bound < j)
  {
    quicksort_serial(left_bound, j);
  }
  if (i < right_bound)
  {
    quicksort_serial(i, right_bound);
  }
  //  if(g_mpi_myrank==0)
  //printf("%d %d %d %d\n",i,j,left_bound,right_bound);

}

/***************************************************************************/
/* Function: Save Image ****************************************************/
/***************************************************************************/

Mat& save_img(Mat& I, int mpi_myrank)
{
  unsigned int curr_x = 0;
  unsigned int curr_y = 0;
  unsigned int i = 1;
  // accept only char type matrices
  CV_Assert(I.depth() == CV_8U);

  int j,k;
  int startX = 0;
  int startY = g_numrows * mpi_myrank;
  int endX = I.cols;
  int endY = startY + g_numrows;

  //MatIterator_<Vec3b> it, end;
  //for( it = I.begin<Vec3b>(), end = I.end<Vec3b>(); it != end; ++it, i++)
  for(j=startY; j<endY; j++) {
    for(k=startX; k<endX; k++)
  {
    curr_y = (i - 1) / width;
    curr_x = (i - 1) % width;
    Vec3b &it = I.at<Vec3b>(j,k);
    (it)[0] = g_matrix[curr_y][curr_x][0];
    (it)[1] = g_matrix[curr_y][curr_x][1];
    (it)[2] = g_matrix[curr_y][curr_x][2];
    i++;
  }
  }

  return I;
}
