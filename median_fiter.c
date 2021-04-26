#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
// #include <sys/wait.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#pragma pack(push, 1) /* diz pro compilador não alterar alinhamento \
                         ou tamanho da struct */

typedef struct pixel
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} PIXEL;

typedef struct bmpHeader
{

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

void medianFilter(int *imageArray, int rows, int cols, int sequential, int numProcesses, int mask);
void mapImageToMatrix(HEADER *header, PIXEL **pixelMap, FILE *file, int **imageMatrix, int rows, int cols);
void mapMatrixToArray(int **imageMatrix, int *array, int rows, int cols);
int freeImageMatrix(int **imageMatrix, int rows);
void error(char filePath[50]);

int cmpfunc(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

int main(int argc, char **argv[])
{

    FILE *bmpImage;
    FILE *outputImage;
    HEADER bmpHeader;
    PIXEL pixel;

    // char inputFilePath[50], outputFilePath[50];
    // int numProcesses;
    // numProcesses             = argv[1];
    // inputFilePath  = argv[2];
    // outputFilePath = argv[3];

    // if (argc != 4) {
    //     printf("%s <NumberProcesses> <InputFile> <OutputFile>\n", argv[0]);
    //     exit(0);
    // }

    char inputFilePath[50] = "images/salt-and-pepper-borboleta.bmp";
    char outputFilePath[50] = "images/testeBorboleta.bmp";

    int shmid;
    int key = 4;

    bmpImage = fopen(inputFilePath, "rb");
    if (bmpImage == NULL)
    {
        error(inputFilePath);
    }

    outputImage = fopen(outputFilePath, "wb");
    if (outputImage == NULL)
    {
        error(outputFilePath);
    }

    fread(&bmpHeader, sizeof(HEADER), 1, bmpImage);

    fwrite(&bmpHeader, sizeof(HEADER), 1, outputImage);

    int rows = bmpHeader.height;
    int cols = bmpHeader.width;
    int arrayLenght = rows * cols;

    // aloca matriz dinamicamente
    int **imageMatrix = (int **)malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++)
        imageMatrix[i] = (int *)malloc(cols * sizeof(int));

    // mapeia pixeis em matrix de pixeis
    PIXEL **pixelMap = (PIXEL **)malloc(sizeof(PIXEL *) * rows);
    for (int i = 0; i < bmpHeader.height; i++)
        pixelMap[i] = (PIXEL *)malloc(sizeof(PIXEL) * bmpHeader.width);

    int *imageMatrixArray; //poderia ser um array de pixeis...
    imageMatrixArray = malloc(sizeof(int) * arrayLenght);

    mapImageToMatrix(&bmpHeader, pixelMap, bmpImage, imageMatrix, rows, cols);
    mapMatrixToArray(imageMatrix, imageMatrixArray, rows, cols);

    medianFilter(imageMatrixArray, rows, cols, 0, 1, 3);

    int arrayIndex;
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {

            arrayIndex = i * cols + j;

            pixelMap[i][j].red = imageMatrixArray[arrayIndex];
            pixelMap[i][j].green = imageMatrixArray[arrayIndex];
            pixelMap[i][j].blue = imageMatrixArray[arrayIndex];
            fwrite(&pixelMap[i][j], sizeof(PIXEL), 1, outputImage);
        }
    }

    freeImageMatrix(imageMatrix, rows);
    fclose(bmpImage);
    fclose(outputImage);
    return 0;
}

void medianFilter(int *imageArray, int rows, int cols, int sequential, int numProcesses, int mask)
{

    int offset = cols * sequential;
    int arrayLength = cols * rows;
    int maskSize = mask * mask;
    int maskArray[maskSize];
    int maskOffsetFromStart = mask / 2;

    while (offset < arrayLength)
    {
        for (int arrayIndex = offset, col = 0; col < cols; col++, arrayIndex++)
        {

            memset(maskArray, 0, maskSize * sizeof(int));

            int maskStartingRow = MAX(offset / rows - maskOffsetFromStart, 0);
            int maskStartingCol = MAX(col - maskOffsetFromStart, 0);

            int maskStartingIndexInArray = maskStartingRow * cols + maskStartingCol;

            int i = 0;
            // fiz uma lógica meio louca para achar um ponto valido da máscara no array.
            // pensei numa maneira de simular que estamos navegando uma matriz, sendo que 
            // na verdade temos apenas um array.
            for (int maskRow = maskStartingRow, j = 0; j < mask; j++, maskRow++)
            {
                for (int maskCol = maskStartingCol, k = 0; k < mask; k++, maskCol++)
                {
                    int arrayIndex = maskRow * cols + maskCol;
                    maskArray[i++] = imageArray[arrayIndex];
                }
            }

            // ordena nosso array de valor para extrairmos a mediana 
            qsort(maskArray, maskSize, sizeof(int), cmpfunc);

            int newValue = maskArray[maskSize / 2];
            imageArray[arrayIndex] = newValue;
        }

        sequential = sequential + numProcesses;
        offset = cols * sequential;
    }
}

void mapImageToMatrix(HEADER *header, PIXEL **pixelMap, FILE *file, int **imageMatrix, int rows, int cols)
{

    /**
     * Esse método vai mapear os valores dos pixeis para uma matriz, assim
     * podemos depois transformar  a matriz em um vetor e guardar seus valores
     * em um espaço de memória compartilhado.
    */

    int average;

    for (int i = 0; i < header->height; i++)
    {
        for (int j = 0; j < header->width; j++)
        {
            fread(&pixelMap[i][j], sizeof(PIXEL), 1, file);
        }
    }

    for (int i = header->height - 1; i >= 0; i--) //poderia fazer isso direto no vetor...
    {
        for (int j = 0; j < header->width; j++)
        {
            average = (pixelMap[i][j].red + pixelMap[i][j].green + pixelMap[i][j].blue) / 3;
            imageMatrix[i][j] = average;
        }
    }
}

void mapMatrixToArray(int **imageMatrix, int *array, int rows, int cols)
{
    int arrayIndex;

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            arrayIndex = i * cols + j;
            array[arrayIndex] = imageMatrix[i][j];
        }
    }
}

int freeImageMatrix(int **imageMatrix, int rows)
{

    for (int i = 0; i < rows; i++)
    {
        free(imageMatrix[i]);
    }

    free(imageMatrix);
}

void printFileDetails(HEADER *header)
{
    //====== PARA DEBUG ======//
    printf("Tamanho da imagem: %u\n", header->fileSize);
    printf("Largura: %d\n", header->width);
    printf("Altura: %d\n", header->height);
    printf("Bits por pixel: %d\n", header->bits);
}

void error(char filePath[50])
{
    printf("Erro ao abrir arquivo %s\n", filePath);
    exit(0);
}
