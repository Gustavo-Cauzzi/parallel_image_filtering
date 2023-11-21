#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
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

void grayscale (CABECALHO cabecalho, FILE *fin, FILE *fout) {
	char aux;
	int i, j;
	short media;
	RGB pixel;


	fread(&cabecalho, sizeof(CABECALHO), 1, fin);
	fwrite(&cabecalho, sizeof(CABECALHO), 1, fout);

	int ali = (cabecalho.largura * 3) % 4;
	if (ali != 0) ali = 4 - ali;
	
	for(i = 0; i < cabecalho.altura; i++){
		for(j = 0; j < cabecalho.largura; j++){
			fread(&pixel, sizeof(RGB), 1, fin);
			media = (pixel.red + pixel.green + pixel.blue) / 3;
			pixel.red = media;
			pixel.green = media;
			pixel.blue = media;
			fwrite(&pixel, sizeof(RGB), 1, fout);
		}

		for(j = 0; j<ali; j++){
			fread(&aux, sizeof(unsigned char), 1, fin);
			fwrite(&aux, sizeof(unsigned char), 1, fout);
		}
	}
}
/*---------------------------------------------------------------------*/
void apply_mask (CABECALHO cabecalho, FILE *fin, FILE *fout) {
	char aux;
	int i, j, k;
	short media;
	RGB pixel;

	fread(&cabecalho, sizeof(CABECALHO), 1, fin);
	fwrite(&cabecalho, sizeof(CABECALHO), 1, fout);

	int ali = (cabecalho.largura * 3) % 4;
	if (ali != 0) ali = 4 - ali;

	RGB** pixels = malloc(sizeof(RGB*) * cabecalho.altura);
	for(i = 0; i < cabecalho.altura; i++){
		pixels[i] = malloc(sizeof(RGB) * cabecalho.largura);
	}
	RGB** pixelsToSave = malloc(sizeof(RGB*) * cabecalho.altura);
	for(i = 0; i < cabecalho.altura; i++){
		pixelsToSave[i] = malloc(sizeof(RGB) * cabecalho.largura);
	}

	for(i = 0; i < cabecalho.altura; i++){
		for(j = 0; j < cabecalho.largura; j++){
			fread(&pixel, sizeof(RGB), 1, fin);
			pixels[i][j] = pixel;
		} 

		for(j = 0; j<ali; j++){
			fread(&aux, sizeof(unsigned char), 1, fin);
		}
	}

	int maskX[3][3] = {{-1, 0, 1}, {-1, 0, 1}, {-1, 0, 1}};
	int maskY[3][3] = {{-1, -1, -1}, { 0,  0,  0}, { 1,  1,  1}};

	#pragma omp parallel for private(j)
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
					sumX += pixels[yPosToCalc][xPosToCalc].red * maskX[y + 1][x + 1];
                    sumY += pixels[yPosToCalc][xPosToCalc].red * maskY[y + 1][x + 1];
				}
			}

			int colorValue = (int) sqrt(sumX * sumX + sumY * sumY);
			colorValue = min(colorValue, 255);
			RGB pixel;
			pixel.blue = colorValue;
			pixel.green = colorValue;
			pixel.red = colorValue;
			pixelsToSave[i][j] = pixel;
		}
	}

	for(i = 0; i < cabecalho.altura; i++){
		for(j = 0; j < cabecalho.largura; j++){
			RGB pixel2 = pixelsToSave[i][j];
			fwrite(&pixel2, sizeof(RGB), 1, fout);
			for(k = 0; k < ali; k++){
				fwrite(&aux, sizeof(unsigned char), 1, fout);
			}
		}
	}

	for(i = 0; i < cabecalho.altura; i++){
		free(pixels[i]);
		free(pixelsToSave[i]);
	}
	free(pixels);
	free(pixelsToSave);

}
/*---------------------------------------------------------------------*/
int main(int argc, char **argv ){

	char entrada[100], saida[100];
	CABECALHO cabecalho;
	FILE *fgray;

    if (argc < 3) {
        printf("%s <caminho_arquivo_entrada> <caminho_arquivo_saida> [<0_1_skip_grayscale>]\n", argv[0]);
        exit(0);
    }


	int skipGrayScale = 0;
	if (argc == 4) {
		skipGrayScale = atoi(argv[3]);
	}

	if (skipGrayScale == 0) {
		FILE *fin = fopen(argv[1], "rb");

		if ( fin == NULL ){
			printf("Erro ao abrir o arquivo %s\n", argv[1]);
			exit(0);
		}  

		fgray = fopen("./gray.bmp", "wb");

		if ( fgray == NULL ){
			printf("Erro ao abrir o arquivo %s\n", argv[2]);
			exit(0);
		}  

		grayscale(cabecalho, fin, fgray);

		fclose(fgray);
		fclose(fin);
	}

	FILE *fout = fopen(argv[2], "wb");

	if ( fout == NULL ){
		printf("Erro ao abrir o arquivo %s\n", argv[2]);
		exit(0);
	}  
	
	fgray = fopen("./gray.bmp", "rb");
	if ( fgray == NULL ){
		printf("Erro ao abrir o arquivo %s\n", argv[2]);
		exit(0);
	}  

	apply_mask(cabecalho, fgray, fout);

	fclose(fgray);
	fclose(fout);
}
/*---------------------------------------------------------------------*/