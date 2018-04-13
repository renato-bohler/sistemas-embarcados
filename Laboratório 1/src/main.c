#include <stdio.h>
#include <stdlib.h>
#define MAX_LINE_LENGTH 70
extern long int meanfilter3(int dim_x, int dim_y, char* img_in, char* img_out);

int main(){
  FILE *fIn = fopen("img_in/teste_performance.ascii.pgm", "r");
  FILE *fOut = fopen("img_out/out.ascii.pgm", "w");
  
  char value;
  char lineRead[MAX_LINE_LENGTH];
  int offset = 0;
  
  // Copia magic word e o nome da imagem
  fgets(lineRead, MAX_LINE_LENGTH, fIn);
  fputs(lineRead, fOut);
  fgets(lineRead, MAX_LINE_LENGTH, fIn);
  fputs(lineRead, fOut);
  
  // Copia as dimensões da imagem original
  int dim_x, dim_y;
  fscanf(fIn, "%d %d\n", &dim_x, &dim_y);
  fprintf(fOut, "%d %d\n", dim_x - 2, dim_y - 2);
  
  // Copia o máximo valor
  fgets(lineRead, MAX_LINE_LENGTH, fIn);
  fputs(lineRead, fOut);
  
  char *img_in = malloc (dim_x * dim_y * sizeof (char));
  
  while(!feof(fIn)){
    fscanf(fIn, "%hu", &value);
    img_in[offset++] = value;
  }
  
  int tam = (dim_x-2) * (dim_y-2);
  char* img_out = malloc (tam * sizeof (char));
  long int j = meanfilter3(dim_x, dim_y, img_in, img_out);
  
  // Escreve o arquivo de saída
  int i;
  for(i = 0; i < j; i++) {
    fprintf(fOut, "%d ", img_out[i]);
  }
  
  fclose(fIn);
  fclose(fOut);
  
  return 0;
}

