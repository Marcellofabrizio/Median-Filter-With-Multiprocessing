#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/shm.h>
// #include <sys/wait.h>


#pragma pack(push, 1) /* diz pro compilador não alterar alinhamento
                         ou tamanho da struct */

typedef struct pixel{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} PIXEL;

typedef struct bmpHeader {

    /*
        int32_t == signed integer
        uint16_t == unsigned short
        uint32_t == unsigned integer
    */

    uint16_t type;

    uint32_t fileSize;

    uint16_t reserved1, reserved2;

    uint32_t offset, header_size;

    int32_t width, height;

    uint16_t planes, bits;

    uint32_t compressionType, imageSize;

    int32_t xResolution, yResolution;

    uint32_t numColours, importantColours;

} HEADER;


void medianFilter(FILE *inputFile, FILE *outputFile) {
    
}

int main(int argc, char **argv[])
{
    
    /** 
     * TODO: Implementar entrada de arquivos como argumento
    */

    FILE *bmpImage;
    FILE *outputImage;
    HEADER bmpHeader;
    PIXEL pixel;

    // char inputFilePath[50], outputFilePath[50];
    // inputFilePath = argv[1];
    // outputFilePath = argv[2];

    // if (argc != 3) {
    //     printf("%s <InputFile> <OutputFile>\n", argv[0]);
    //     exit(0);
    // }

    char inputFilePath[50]  = "images/borboleta.bmp";
    char outputFilePath[50] = "images/outputfile.bmp";

    int shmid;
    int arrayIndex;
    int key = 4;
    int *partial_image = NULL;

    bmpImage = fopen(inputFilePath, "rb");
    if(bmpImage == NULL) {
        error(inputFilePath);
    }
    
    outputImage = fopen(outputFilePath, "wb");
    if(outputImage == NULL) {
        error(outputFilePath);
    }

    fread(&bmpHeader, sizeof(HEADER), 1, bmpImage);
    
    printf("Tamanho da imagem: %u\n", bmpHeader.fileSize);
	printf("Largura: %d\n", bmpHeader.width);
	printf("Altura: %d\n", bmpHeader.height);
	printf("Bits por pixel: %d\n", bmpHeader.bits);

    int rows = bmpHeader.height;
    int cols = bmpHeader.width;

    int arrayLenght = rows*cols;
    int **Matrix = (int **)malloc(rows*sizeof(int *));

    for (int i = 0; i < rows; i++)
        Matrix[i] = (int *)malloc(cols*sizeof(int));

    mapImageToMatrix(bmpHeader, bmpImage, Matrix, rows, cols);
    
    int length = rows*cols;
    int *imageMatrixVector; 

    //liga o vetor da matriz no espaço de memória compartilhado
    shmid = shmget(key, length*sizeof(int), IPC_CREAT | 0600);
    imageMatrixVector = (int *)shmat(shmid, 0, 0);

    imageMatrixVector = malloc(sizeof(int)*arrayLenght);

    //mapeia matriz em um vetor
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            arrayIndex = i*cols + j;
            imageMatrixVector[arrayIndex] = Matrix[i][j];
        }
    }

    int pid;
    int id = 0;
    for(int i = 0; i < 2; i++) {
        pid = fork();
        if (pid == 0) {
            id = i;
            break;
        }
    }

    if (id != 0) {
        printf("Sou o processo filho\n");
        for(int i = 0; i < length; i++) {
            printf("%d", imageMatrixVector[i]);
        }
    } 

    freeImageMatrix(Matrix, rows);
    fclose(bmpImage);

    return 0;
}

void mapImageToMatrix(HEADER header, FILE *file, int **Matrix, int rows, int cols) {

    /**
     * Esse método vai mapear os valores dos pixeis para uma matriz, assim
     * podemos depois transformar  a matriz em um vetor e guardar seus valores
     * em um espaço de memória compartilhado.
    */

    short average;
    PIXEL pixel;

    for(int i = 0; i < header.height; i++) {
        for(int j = 0; j < header.width; j++) {
            fread(&pixel, sizeof(PIXEL), 1, file);
            average = (pixel.red + pixel.green + pixel.blue)/3;
            Matrix[i][j] = average;
        }
    }

}

int freeImageMatrix(int **Matrix, int rows) {

    for(int i = 0; i < rows; i++) {
        free(Matrix[i]);
    }

    free(Matrix);

}

void error(char filePath[50]){
    printf("Erro ao abrir arquivo %s\n", filePath);
    exit(0);
}
