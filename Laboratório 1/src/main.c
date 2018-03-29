extern long int meanfilter3(int dim_x, int dim_y, char* img_in, char* img_out);

#include <stdlib.h>

int main(){

  char *img_in = malloc (160 * 120 * sizeof (char));

  int dim_x = 160;
  int dim_y = 120;
  
  int tam = (dim_x-2) * (dim_y-2);
  char* img_out = malloc (tam * sizeof (char));
  long int j = meanfilter3(dim_x, dim_y, img_in, img_out);
  
  return j;
}

