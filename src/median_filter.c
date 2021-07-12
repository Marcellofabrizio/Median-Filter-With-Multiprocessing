#include "median_filter.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

char aux;

int cmpfunc(const void *a, const void *b)
{
    // função de comparação para o quick sort
    int totalA = (((PIXEL *)a)->red + ((PIXEL *)a)->green + ((PIXEL *)a)->blue);
    int totalB = (((PIXEL *)b)->red + ((PIXEL *)b)->green + ((PIXEL *)b)->blue);
    return totalB - totalA;
}

FILE *openFile(char filePath[50], char mode[3])
{
    FILE *file = fopen(filePath, mode);
    if (file == NULL)
    {
        error(filePath, NULL);
    }
}

void *medianFilter_args(void *args)
{
    // PIXEL *imageArray, int rows, int cols, int sequential, int numInstances, int mask

    ARGUMENTS *argument = (ARGUMENTS *)args;

    PIXEL *imageArray = argument->imageArray;
    int rows = argument->rows;
    int cols = argument->cols;
    int sequential = argument->sequential;
    int numInstances = argument->numInstances;
    int mask = argument->mask;

    int offset = cols * sequential;
    long arrayLength = cols * rows;
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
            for (int maskRow = maskStartingRow, j = 0; j < mask && j < rows; j++, maskRow++)
            {
                for (int maskCol = maskStartingCol, k = 0; k < mask && k < cols; k++, maskCol++)
                {
                    long arrayIndex = maskRow * cols + maskCol;
                    maskArray[i++] = imageArray[arrayIndex];
                }
            }

            qsort(maskArray, maskSize, sizeof(PIXEL), cmpfunc);
            PIXEL newValue = {0, 0, 0};
            newValue.red = maskArray[maskSize / 2].red;
            newValue.green = maskArray[maskSize / 2].green;
            newValue.blue = maskArray[maskSize / 2].blue;

            imageArray[arrayIndex].red = newValue.red;
            imageArray[arrayIndex].green = newValue.green;
            imageArray[arrayIndex].blue = newValue.blue;
        }

        sequential = sequential + numInstances;
        offset = cols * sequential;
    }
}

void medianFilter(PIXEL *imageArray, int rows, int cols, int sequential, int numProcesses, int mask)
{

    int offset = cols * sequential;
    long arrayLength = cols * rows;
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

            for (int maskRow = maskStartingRow, j = 0; j < mask && j < rows; j++, maskRow++)
            {
                for (int maskCol = maskStartingCol, k = 0; k < mask && k < cols; k++, maskCol++)
                {
                    long arrayIndex = maskRow * cols + maskCol;
                    maskArray[i++] = imageArray[arrayIndex];
                }
            }

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

void mapImageToArray(HEADER *header, PIXEL **pixelBitmap, PIXEL *pixels, FILE *file)
{

    /**
     * Esse método vai mapear os valores dos pixeis para um array de pixeis, assim
     * podemos guardar seus valores em um espaço de memória compartilhado.
    */
    // long arrayIndex;
    // PIXEL pixel;

    // for (int i = 0; i < header->height; i++)
    // {
    //     for (int j = 0; j < header->width; j++)
    //     {
    //         fread(&pixel, sizeof(PIXEL), 1, file);
    //         arrayIndex = i * header->width + j;
    //         pixels[arrayIndex].red = pixel.red;
    //         pixels[arrayIndex].green = pixel.green;
    //         pixels[arrayIndex].blue = pixel.blue;
    //     }
    // }
}

void writeImage(HEADER *bmpHeader, PIXEL *pixels, FILE *file, int numInstances, char outputFilePath[100], int cols, int arrayLength)
{

    FILE *outputImage;

    outputImage = openFile(outputFilePath, "wb");
    fwrite(&bmpHeader, sizeof(HEADER), 1, outputImage);

    for (int i = 1; i < numInstances; i++)
    {
        wait(NULL);
    }

    long arrayIndex;
    PIXEL pixel;

    int alignment = (cols * 3) % 4;
    if (alignment != 0)
    {
        alignment = 4 - alignment;
    }

    for (int i = 0; i < arrayLength; i++)
    {
        pixel.red = pixels[i].red;
        pixel.green = pixels[i].green;
        pixel.blue = pixels[i].blue;
        fwrite(&pixel, sizeof(PIXEL), 1, outputImage);
    }

    for (int j = 0; j < alignment; j++)
    {
        fread(&aux, sizeof(unsigned char), 1, file);
        fwrite(&aux, sizeof(unsigned char), 1, outputImage);
    }

    fclose(file);
    fclose(outputImage);
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

void error(char filePath[50], char message[100])
{
    printf("Erro ao abrir arquivo %s\n", filePath);
    if (message)
        printf("%s\n", message);
    exit(0);
}
