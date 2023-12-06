#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// mpicc ./mascMpi.c -o mascMpi -lm
// time mpirun -np 3 ./mascMpi ./borboleta.bmp ./outMpi.bmp

/*---------------------------------------------------------------------*/
#pragma pack(1)
#define min(a,b) a < b ? a : b
/*---------------------------------------------------------------------*/
struct cabecalho {
	unsigned short tipo;
	unsigned int tamanho_arquivo;
	unsigned short reservado1;
	unsigned short reservado2;
	unsigned int offset;
	unsigned int tamanho_image_header;
	int largura;
	int altura;
	unsigned short planos;
	unsigned short bits_por_pixel;
	unsigned int compressao;
	unsigned int tamanho_imagem;
	int largura_resolucao;
	int altura_resolucao;
	unsigned int numero_cores;
	unsigned int cores_importantes;
}; 
typedef struct cabecalho CABECALHO;

struct rgb{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
};
typedef struct rgb RGB;
/*---------------------------------------------------------------------*/
void apply_mask (CABECALHO cabecalho, FILE *fin, FILE *fout, int id, int np) {
	char aux;
	int i, j, k;
	short media;
	RGB pixel;

	fread(&cabecalho, sizeof(CABECALHO), 1, fin);
	fwrite(&cabecalho, sizeof(CABECALHO), 1, fout);

	int ali = (cabecalho.largura * 3) % 4;
	if (ali != 0) ali = 4 - ali;
	int blockHeight = cabecalho.altura / np;

	short* pixels = malloc(sizeof(short) * cabecalho.altura * cabecalho.largura);
 	short* pixelsBuffer = malloc(sizeof(short) * blockHeight * cabecalho.largura);
 	short* pixelsToSave = NULL;

	double ti = MPI_Wtime();
	if (id == 0) {
		pixelsToSave = malloc(sizeof(short) * cabecalho.altura * cabecalho.largura);
		for(i = 0; i < cabecalho.altura; i++){
			for(j = 0; j < cabecalho.largura; j++){
				fread(&pixel, sizeof(RGB), 1, fin);
				media = (pixel.red + pixel.green + pixel.blue) / 3;
				pixels[i * cabecalho.largura + j] = media;
			} 

			for(j = 0; j<ali; j++){
				fread(&aux, sizeof(unsigned char), 1, fin);
			}
		}
	}
		

	MPI_Bcast(pixels, cabecalho.altura * cabecalho.largura, MPI_SHORT,0,MPI_COMM_WORLD);

	int maskX[3][3] = {{-1, 0, 1}, {-1, 0, 1}, {-1, 0, 1}};
	int maskY[3][3] = {{-1, -1, -1}, { 0,  0,  0}, { 1,  1,  1}};
	for(i = 0; i < blockHeight; i++){
		for(j = 0; j < cabecalho.largura; j++){
			int x, y;
			int sumX = 0, sumY = 0;

			for (y = -1; y <= 1; y++) {
				for (x = -1; x <= 1; x++) {
					int xPosToCalc = j + x;
					int yPosToCalc = i + y;

					if (
						(id == 0 && yPosToCalc < 0) 
						|| (id == np - 1 && yPosToCalc >= cabecalho.altura)
						|| xPosToCalc < 0 || xPosToCalc >= cabecalho.largura
					) continue;

					sumX += pixels[(blockHeight * id + yPosToCalc) * cabecalho.largura + xPosToCalc] * maskX[y + 1][x + 1];
                    sumY += pixels[(blockHeight * id + yPosToCalc) * cabecalho.largura + xPosToCalc] * maskY[y + 1][x + 1];
				}
			}

			if (i == 0 || i == blockHeight - 1) {
				float mediaY = sumY / 3;
				sumY += mediaY * -3;
			}
			if (j == 0 || j == cabecalho.largura - 1) {
				float mediaX = sumX / 3;
				sumX += mediaX * -3;
			}

			int colorValueRaw = (int) sqrt(sumX * sumX + sumY * sumY);
			short colorValue = (short) min(colorValueRaw, 255);
			pixelsBuffer[i * cabecalho.largura + j] = colorValue;
		}
	}

	MPI_Gather(pixelsBuffer, blockHeight * cabecalho.largura, MPI_SHORT, 
				pixelsToSave, blockHeight * cabecalho.largura, MPI_SHORT, 0, MPI_COMM_WORLD);

	double tf = MPI_Wtime();

	if (id == 0) {
		printf("Time: %lf\n", tf - ti);
		for(i = 0; i < cabecalho.altura; i++){
			for(j = 0; j < cabecalho.largura; j++){
				short color = pixelsToSave[i * cabecalho.largura + j];
				RGB pixel;
				pixel.blue = color;
				pixel.green = color;
				pixel.red = color;
				fwrite(&pixel, sizeof(RGB), 1, fout);
				for(k = 0; k < ali; k++){
					fwrite(&aux, sizeof(unsigned char), 1, fout);
				}
			}
		}
	}
	
	free(pixels);
	free(pixelsBuffer);
	free(pixelsToSave);

}
/*---------------------------------------------------------------------*/
void stop_process () {
	MPI_Finalize();
	exit(0);
}
/*---------------------------------------------------------------------*/
int main(int argc, char **argv ){

	char entrada[100], saida[100];
	int id, np;
	CABECALHO cabecalho;
	FILE *fgray;


	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &np);	

    if (id == 0 && argc < 2) {
        printf("%s <caminho_arquivo_entrada> <caminho_arquivo_saida>\n", argv[0]);
		stop_process();
    }

	FILE *fin = fopen(argv[1], "rb");
	FILE *fout = fopen(argv[2], "wb");

	if (id == 0) {
		if ( fin == NULL ){
			printf("Erro ao abrir o arquivo %s\n", argv[1]);
			stop_process();
		}  

		if ( fout == NULL ){
			printf("Erro ao abrir o arquivo %s\n", argv[2]);
			stop_process();
		}  
	}
	
	apply_mask(cabecalho, fin, fout, id, np);

	fclose(fin);
	fclose(fout);

	MPI_Finalize();

}
/*---------------------------------------------------------------------*/