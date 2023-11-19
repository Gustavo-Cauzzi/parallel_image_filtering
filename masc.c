#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

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

	printf("Tamanho da imagem: %u\n", cabecalho.tamanho_arquivo);
	printf("Largura: %d\n", cabecalho.largura);
	printf("Largura: %d\n", cabecalho.altura);
	printf("Bits por pixel: %d\n", cabecalho.bits_por_pixel);

	fwrite(&cabecalho, sizeof(CABECALHO), 1, fout);

	int ali = (cabecalho.largura * 3) % 4;
	if (ali != 0) ali = 4 - ali;
	
	for(i=0; i<cabecalho.altura; i++){
		for(j=0; j<cabecalho.largura; j++){
			fread(&pixel, sizeof(RGB), 1, fin);
			media = (pixel.red + pixel.green + pixel.blue) / 3;
			pixel.red = media;
			pixel.green = media;
			pixel.blue = media;
			fwrite(&pixel, sizeof(RGB), 1, fout);
		}

		for(j=0; j<ali; j++){
			fread(&aux, sizeof(unsigned char), 1, fin);
			fwrite(&aux, sizeof(unsigned char), 1, fout);
		}
	}
}
/*---------------------------------------------------------------------*/
void apply_mask (CABECALHO cabecalho, FILE *fin, FILE *fout) {
	char aux;
	int i, j;
	short media;
	RGB pixel;
	RGB* pixels = (RGB*) malloc(sizeof(RGB) * cabecalho.largura * cabecalho.altura);
	// RGB* pixels[400 * 600];

	printf("apply_mask\n");
	fread(&cabecalho, sizeof(CABECALHO), 1, fin);

	printf("Tamanho da imagem: %u\n", cabecalho.tamanho_arquivo);
	printf("Largura: %d\n", cabecalho.largura);
	printf("Largura: %d\n", cabecalho.altura);
	printf("Bits por pixel: %d\n", cabecalho.bits_por_pixel);

	fwrite(&cabecalho, sizeof(CABECALHO), 1, fout);

	int ali = (cabecalho.largura * 3) % 4;
	if (ali != 0) ali = 4 - ali;
	printf("%d\n\n", (int) ((cabecalho.largura * cabecalho.altura)/sizeof(RGB)));

    fread(&pixels, sizeof(RGB), cabecalho.largura * cabecalho.altura, fin);

	// for(i=0; i<cabecalho.altura; i++){
	// 	for(j=0; j<cabecalho.largura; j++){
	// 		fread(&pixel, sizeof(RGB), 1, fin);
	// 		int pos = i * cabecalho.largura + j;
	// 		// printf("%d - ", pos);
	// 		RGB memPix = pixels[pos];
	// 		memPix.blue = pixel.blue;
	// 		memPix.green = pixel.green;
	// 		memPix.red = pixel.red;
	// 		pixels[i * cabecalho.largura + j] = memPix;
	// 	} 

	// 	for(j=0; j<ali; j++){
	// 		fread(&aux, sizeof(unsigned char), 1, fin);
	// 	}
	// }
	for(i=0; i<cabecalho.altura; i++){
		for(j=0; j<cabecalho.largura; j++){
			printf("asdbui %d\n", i * cabecalho.largura + j);
			printf("RGB %d %d %d \n", pixels[i * cabecalho.largura + j].red, pixels[i * cabecalho.largura + j].green, pixels[i * cabecalho.largura + j].blue);
		}
	}

	printf("\n\nAplicando filtro\n\n");
	int maskX[3][3] = { {-1, 0, 1}, {-1, 0, 1}, {-1, 0, 1}};
	int maskY[3][3] = {{-1, -1, -1}, { 0,  0,  0}, { 1,  1,  1}};

	for(i=0; i<cabecalho.altura; i++){
		for(j=0; j<cabecalho.largura; j++){
			int x, y;
			int sumX, sumY;

			for (y = -1; y <= 1; y++) {
				for (x = -1; x <= 1; x++) {
					int xPosToCalc = j + x;
					int yPosToCalc = i + y;

					if (
						yPosToCalc < 0 || yPosToCalc >= cabecalho.altura
						|| xPosToCalc < 0 || xPosToCalc >= cabecalho.largura
					) continue;
					int posToCalc = yPosToCalc * cabecalho.largura + xPosToCalc;
					// printf("%d %d - %d %d r%d |", j, i, xPosToCalc, yPosToCalc, pixels[posToCalc].red);
					sumX += pixels[posToCalc].red * maskX[y][x];
                    sumY += pixels[posToCalc].red * maskY[y][x];
				}
			}

			int colorValue = (int) sqrt(sumX * sumX + sumY * sumY);
			// printf("n%d %d =Novo: %d", sumX, sumY, colorValue);
			int pos = i * cabecalho.largura + j;
			colorValue = min(colorValue, 255);
			RGB pixel = pixels[pos];
			pixel.blue = colorValue;
			pixel.green = colorValue;
			pixel.red = colorValue;
			pixels[pos] = pixel;
			// printf(" ==========\n");
		}
	}

	
	printf("\n\n\n\nSalvando\n\n\n\n");
	for(i=0; i<cabecalho.altura; i++){
		for(j=0; j<cabecalho.largura; j++){
			RGB memPix = pixels[i * cabecalho.largura + j];
			// printf("%d %d %d | ", memPix.red, memPix.green, memPix.blue);
			fwrite(&memPix, sizeof(RGB), 1, fout);
		}
		for(j=0; j<ali; j++){
			fwrite(&aux, sizeof(unsigned char), 1, fout);
		}
	}
	free(pixels);

}
/*---------------------------------------------------------------------*/
int main(int argc, char **argv ){

	char entrada[100], saida[100];
	CABECALHO cabecalho;

    if (argc != 3) {
        printf("%s <caminho_arquivo_entrada> <caminho_arquivo_saida>\n", argv[0]);
        exit(0);
    }


	FILE *fin = fopen(argv[1], "rb");

	if ( fin == NULL ){
		printf("Erro ao abrir o arquivo %s\n", argv[1]);
		exit(0);
	}  

	FILE *fgray = fopen("./gray.bmp", "wb");

	if ( fgray == NULL ){
		printf("Erro ao abrir o arquivo %s\n", argv[2]);
		exit(0);
	}  

	FILE *fout = fopen(argv[2], "wb");

	if ( fout == NULL ){
		printf("Erro ao abrir o arquivo %s\n", argv[2]);
		exit(0);
	}  

	grayscale(cabecalho, fin, fgray);

	fclose(fgray);
	fgray = fopen("./gray.bmp", "rb");

	apply_mask(cabecalho, fgray, fout);

	fclose(fgray);
	fclose(fin);
	fclose(fout);
}
/*---------------------------------------------------------------------*/