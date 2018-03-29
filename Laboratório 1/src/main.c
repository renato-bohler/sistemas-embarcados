extern long int meanfilter3(int dim_x, int dim_y, char* img_in, char* img_out);

#include <stdlib.h>

int main(){

  char img_in[] = {10, 01, 01, 02, 20,
                   01, 01, 01, 03, 04,
                   01, 01, 01, 04, 04,
                   02, 03, 04, 04, 05,
                   03, 04, 04, 05, 05,
                   30, 04, 05, 05, 40};

  int dim_x = 5;
  int dim_y = 6;
  
  int tam = (dim_x-2) * (dim_y-2);
  char* img_out = malloc (tam * sizeof (char));
  long int j = meanfilter3(dim_x, dim_y, img_in, img_out); //to do
  free (img_out);
  img_out = NULL;
  
  return j;
}

