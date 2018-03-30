#include <stdio.h>
#include <stdlib.h>

extern long int meanfilter3(int dim_x, int dim_y, char* img_in, char* img_out);

int main(){

  FILE *fIn = fopen("img_in/f14.ascii.pgm", "r");
  // TODO: fazer escrever a imagem de saída
  FILE *fOut = fopen("img_out/out.ascii.pgm", "ab+");
  
  char value;
  int offset = 0;
  
  // Desconsidera magic word e o nome da imagem
  fscanf(fIn, "%s\n", &value);
  fscanf(fIn, "%s\n", &value);
  
  int dim_x, dim_y;
  fscanf(fIn, "%d %d\n", &dim_x, &dim_y);
  // Desconsidera o máximo valor
  fscanf(fIn, "%s\n", NULL);
  
  char *img_in = malloc (dim_x * dim_y * sizeof (char));
  
  while(!feof(fIn)){
    fscanf(fIn, "%hu", &value);
    fprintf(fOut, "%hu", value);
    img_in[offset] = value;
  }
  
  int tam = (dim_x-2) * (dim_y-2);
  char* img_out = malloc (tam * sizeof (char));
  long int j = meanfilter3(dim_x, dim_y, img_in, img_out);
  
  return j;
}

