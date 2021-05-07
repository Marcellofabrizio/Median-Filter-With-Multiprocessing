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

char aux;

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

// ========================================== PROTÓTIPOS ==========================================
FILE *openFile(char filePath[50], char mode[3]);
void printFileDetails(HEADER header);
void allocPixelImageMatrix(PIXEL ***pixelBitmap, int rows, int cols);
void deallocPixelBitmap(PIXEL **pixelBitmap, int rows);
void medianFilter(PIXEL *imageArray, int rows, int cols, int sequential, int numProcesses, int mask);
void mapImageToArray(HEADER *header, PIXEL **pixelBitmap, PIXEL *pixels, FILE *file, FILE *fileout);
void error(char filePath[50]);
// =================================================================================================

int cmpfunc(const void *a, const void *b)
{
    // função de comparação para o quick sort
    int totalA = (((PIXEL *)a)->red + ((PIXEL *)a)->green + ((PIXEL *)a)->blue) / 3;
    int totalB = (((PIXEL *)b)->red + ((PIXEL *)b)->green + ((PIXEL *)b)->blue) / 3;
    return totalB - totalA;
}

int main(int argc, char **argv[])
{

    // char inputFilePath[50], outputFilePath[50];
    // int numProcesses;
    // numProcesses             = argv[1];
    // inputFilePath  = argv[2];
    // outputFilePath = argv[3];

    // if (argc != 4) {
    //     printf("%s <NumberProcesses> <InputFile> <OutputFile>\n", argv[0]);
    //     exit(0);
    // }

    FILE *bmpImage;
    FILE *outputImage;
    HEADER bmpHeader;

    char inputFilePath[50] = "images/teste2.bmp";
    char outputFilePath[50] = "images/teste12.bmp";

    int shmid;
    int key = 4;

    bmpImage = openFile(inputFilePath, "rb");

    outputImage = openFile(outputFilePath, "wb");

    fread(&bmpHeader, sizeof(HEADER), 1, bmpImage);

    fwrite(&bmpHeader, sizeof(HEADER), 1, outputImage);

    int rows = bmpHeader.height;
    int cols = bmpHeader.width;
    int arrayLenght = rows * cols;

    PIXEL **pixelBitmap = NULL;
    allocPixelImageMatrix(&pixelBitmap, rows, cols);

    PIXEL *pixels = malloc(sizeof(PIXEL) * arrayLenght);
    //shmid = shmget(key, sizeof(PIXEL) * arrayLenght, IPC_CREAT | 0644);
    //pixels = (PIXEL *)shmat(shmid, NULL, 0);

    mapImageToArray(&bmpHeader, pixelBitmap, pixels, bmpImage, outputImage);

    //===== FAZER EM MULTIPROCESSAMENTO =====
    medianFilter(pixels, rows, cols, 0, 1, 3);
    //=======================================

    deallocPixelBitmap(pixelBitmap, rows);
    allocPixelImageMatrix(&pixelBitmap, rows, cols);

    int arrayIndex;
    PIXEL pixel;
    for (int i = 0; i < rows; i++)
    {

        int alignment = (cols * 3) % 4;
        if (alignment != 0)
        {
            alignment = 4 - alignment;
        }

        for (int j = 0; j < cols; j++)
        {

            arrayIndex = i * cols + j;

            pixelBitmap[i][j].red = pixels[arrayIndex].red;
            pixelBitmap[i][j].green = pixels[arrayIndex].green;
            pixelBitmap[i][j].blue = pixels[arrayIndex].blue;
            fwrite(&pixelBitmap[i][j], sizeof(PIXEL), 1, outputImage);
        }

        for (int j = 0; j < alignment; j++)
        {
            fwrite(&pixelBitmap[i][j], sizeof(unsigned char), 1, outputImage);
        }
    }

    deallocPixelBitmap(pixelBitmap, rows);
    fclose(bmpImage);
    fclose(outputImage);
    return 0;
}

FILE *openFile(char filePath[50], char mode[3])
{
    FILE *file = fopen(filePath, mode);
    if (file == NULL)
    {
        error(filePath);
    }
}

void medianFilter(PIXEL *imageArray, int rows, int cols, int sequential, int numProcesses, int mask)
{

    int offset = cols * sequential;
    int arrayLength = cols * rows;
    int maskSize = mask * mask;
    PIXEL *maskArray = NULL;
    int maskOffsetFromStart = mask / 2;

    while (offset < arrayLength)
    {
        for (int arrayIndex = offset, col = 0; col < cols; col++, arrayIndex++)
        {
            maskArray = (PIXEL *)realloc(maskArray, maskSize * sizeof(PIXEL));

            int maskStartingRow = MAX(offset / cols - maskOffsetFromStart, 0);
            int maskStartingCol = MAX(col - maskOffsetFromStart, 0);

            int maskStartingIndexInArray = maskStartingRow * cols + maskStartingCol;

            int i = 0;
            // fiz uma lógica para achar um ponto valido da máscara no array.
            // pensei numa maneira de simular que estamos navegando uma matriz, sendo que
            // na verdade temos apenas um array.
            for (int maskRow = maskStartingRow, j = 0; j < mask && j < rows; j++, maskRow++)
            {
                for (int maskCol = maskStartingCol, k = 0; k < mask && k < cols; k++, maskCol++)
                {
                    int arrayIndex = maskRow * cols + maskCol;
                    maskArray[i++] = imageArray[arrayIndex];
                }
            }

            // ordena nosso array de valor para extrairmos a mediana

            qsort(maskArray, maskSize, sizeof(PIXEL), cmpfunc);
            PIXEL newValue = {0, 0, 0};
            newValue.red = maskArray[maskSize / 2].red;
            newValue.green = maskArray[maskSize / 2].green;
            newValue.blue = maskArray[maskSize / 2].blue;

            imageArray[arrayIndex].red = newValue.red;
            imageArray[arrayIndex].green = newValue.green;
            imageArray[arrayIndex].blue = newValue.blue;
        }

        sequential = sequential + numProcesses;
        offset = cols * sequential;
    }
}

void mapImageToArray(HEADER *header, PIXEL **pixelBitmap, PIXEL *pixels, FILE *file, FILE *fileout)
{

    /**
     * Esse método vai mapear os valores dos pixeis para um array de pixeis, assim
     * podemos guardar seus valores em um espaço de memória compartilhado.
    */

    int arrayIndex;

    printFileDetails((*header));

    for (int i = 0; i < header->height; i++)
    {
        for (int j = 0; j < header->width; j++)
        {
            fread(&pixelBitmap[i][j], sizeof(PIXEL), 1, file);
            arrayIndex = i * header->width + j;
            pixels[arrayIndex].red = pixelBitmap[i][j].red;
            pixels[arrayIndex].green = pixelBitmap[i][j].green;
            pixels[arrayIndex].blue = pixelBitmap[i][j].blue;
        }
    }


}

void allocPixelImageMatrix(PIXEL ***pixelBitmap, int rows, int cols)
{
    // Aloca dinamicamente uma matrix de pixels
    int i = 0;
    *pixelBitmap = (PIXEL **)malloc(sizeof(PIXEL *) * rows);
    for (i = 0; i < rows; i++)
        (*pixelBitmap)[i] = (PIXEL *)malloc(sizeof(PIXEL) * cols);
}

void deallocPixelBitmap(PIXEL **pixelBitmap, int rows)
{
    for (int i = 0; i < rows; i++)
    {
        free(pixelBitmap[i]);
    }

    free(pixelBitmap);
}

void printFileDetails(HEADER header)
{
    //====== PARA DEBUG ======//
    printf("Tamanho da imagem: %u\n", header.fileSize);
    printf("Largura: %d\n", header.width);
    printf("Altura: %d\n", header.height);
    printf("Bits por pixel: %d\n", header.bits);
}

void error(char filePath[50])
{
    printf("Erro ao abrir arquivo %s\n", filePath);
    exit(0);
}
