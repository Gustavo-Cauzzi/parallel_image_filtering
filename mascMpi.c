#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// gcc ./masc.c -o ./masc -fopenmp -lm
// time ./masc ./borboleta.bmp  ./out.bmp

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

	short* pixels = malloc(sizeof(short) * cabecalho.altura * cabecalho.largura);
 	RGB* pixelsToSave;

	if (id == 0) {
		pixelsToSave = malloc(sizeof(RGB) * cabecalho.altura * cabecalho.largura);
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

	MPI_Bcast(pixels, cabecalho.altura * cabecalho.largura, MPI_FLOAT,0,MPI_COMM_WORLD);

	for (i = 0; i < 20; i++) {
		printf("(%d %d) ", id, pixels[i]);
	}

	int maskX[3][3] = {{-1, 0, 1}, {-1, 0, 1}, {-1, 0, 1}};
	int maskY[3][3] = {{-1, -1, -1}, { 0,  0,  0}, { 1,  1,  1}};

	for(i = 0; i < cabecalho.altura; i++){
		for(j = 0; j < cabecalho.largura; j++){
			int x, y;
			int sumX = 0, sumY = 0;

			for (y = -1; y <= 1; y++) {
				for (x = -1; x <= 1; x++) {
					int xPosToCalc = j + x;
					int yPosToCalc = i + y;

					if (
						yPosToCalc < 0 || yPosToCalc >= cabecalho.altura
						|| xPosToCalc < 0 || xPosToCalc >= cabecalho.largura
					) continue;
					sumX += pixels[yPosToCalc * cabecalho.largura + xPosToCalc] * maskX[y + 1][x + 1];
                    sumY += pixels[yPosToCalc * cabecalho.largura + xPosToCalc] * maskY[y + 1][x + 1];
				}
			}

			int colorValue = (int) sqrt(sumX * sumX + sumY * sumY);
			colorValue = min(colorValue, 255);
			RGB pixel;
			pixel.blue = colorValue;
			pixel.green = colorValue;
			pixel.red = colorValue;
			pixelsToSave[i * cabecalho.largura + j] = pixel;
		}
	}

	for(i = 0; i < cabecalho.altura; i++){
		for(j = 0; j < cabecalho.largura; j++){
			RGB pixel2 = pixelsToSave[i * cabecalho.largura + j];
			fwrite(&pixel2, sizeof(RGB), 1, fout);
			for(k = 0; k < ali; k++){
				fwrite(&aux, sizeof(unsigned char), 1, fout);
			}
		}
	}
	
	free(pixels);
	free(pixelsToSave);

}
/*---------------------------------------------------------------------*/
int main(int argc, char **argv ){

	char entrada[100], saida[100];
	int id, np;
	CABECALHO cabecalho;
	FILE *fgray;


	printf("Ewaidniwon\n");
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &np);	

    if (id == 0 && argc < 2) {
        printf("%s <caminho_arquivo_entrada> <caminho_arquivo_saida>\n", argv[0]);
		MPI_Finalize();
        exit(0);
    }

	FILE *fin = fopen(argv[1], "rb");
	FILE *fout = fopen(argv[2], "wb");

	if (id == 0) {
		if ( fin == NULL ){
			printf("Erro ao abrir o arquivo %s\n", argv[1]);
			exit(0);
		}  

		if ( fout == NULL ){
			printf("Erro ao abrir o arquivo %s\n", argv[2]);
			exit(0);
		}  
	}
	
	// printf("daoapmsda %d %ld\n", id, fin);
	apply_mask(cabecalho, fin, fout, id, np);
	MPI_Finalize();


	fclose(fin);
	fclose(fout);
}
/*---------------------------------------------------------------------*/