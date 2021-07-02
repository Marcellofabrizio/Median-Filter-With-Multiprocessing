#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>

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
void mapImageToArray(HEADER *header, PIXEL **pixelBitmap, PIXEL *pixels, FILE *file);
void error(char filePath[50], char message[100]);
// =================================================================================================

int cmpfunc(const void *a, const void *b)
{
    // função de comparação para o quick sort
    int totalA = (((PIXEL *)a)->red + ((PIXEL *)a)->green + ((PIXEL *)a)->blue) / 3;
    int totalB = (((PIXEL *)b)->red + ((PIXEL *)b)->green + ((PIXEL *)b)->blue) / 3;
    return totalB - totalA;
}

const char *getFilenameExt(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}

int isBmpFile(const char *filename)
{
    const char *ext = getFilenameExt(filename);

    return strcmp(ext, "bmp");
}

int main(int argc, char **argv)
{

    // char inputFilePath[50], outputFilePath[50];

    if (argc != 4)
    {
        printf("<NumberProcesses> <MaskSize> <InputFilePath>\n");
        exit(0);
    }

    int numProcesses, mask;
    numProcesses = atoi(argv[1]);
    mask = atoi(argv[2]);

    char *inputFilePath = argv[3];
    char *outputFilePath = "images/results/correctedImage.bmp";

    FILE *bmpImage;
    FILE *outputImage;
    HEADER bmpHeader;

    int i, j;
    int shmid;
    int pid, seq;
    int key = 4;

    bmpImage = openFile(inputFilePath, "rb");

    if (isBmpFile(inputFilePath))
    {
        error(inputFilePath, "Arquivo deve ser do tipo .bmp");
    }

    fread(&bmpHeader, sizeof(HEADER), 1, bmpImage);

    int rows = bmpHeader.height;
    int cols = bmpHeader.width;
    int arrayLenght = rows * cols;

    PIXEL **pixelBitmap = NULL;
    allocPixelImageMatrix(&pixelBitmap, rows, cols);

    PIXEL *pixels = NULL;

    // Aloca espaço de memória compartilhado entre os processos
    shmid = shmget(key, sizeof(PIXEL) * arrayLenght, IPC_CREAT | 0644);
    pixels = (PIXEL *)shmat(shmid, NULL, 0);

    mapImageToArray(&bmpHeader, pixelBitmap, pixels, bmpImage);

    //=========  MULTIPROCESSAMENTO =========

    seq = 0;

    for (i = 1; i < numProcesses; i++)
    {
        pid = fork();
        if (pid == 0)
        { //processo filho
            seq = i;
            break;
        }
    }

    // mesmo sem filtro, deslocamento continua acontecendo
    medianFilter(pixels, rows, cols, seq, numProcesses, mask);

    if (seq != 0) //se for filho
    {
        shmdt(pixels);
    }
    else if (seq == 0) //se for pai
    {
        // filhos não caem aqui dentro, não entendo porque o número
        // de processos está causando isso...

        outputImage = openFile(outputFilePath, "wb");
        fwrite(&bmpHeader, sizeof(HEADER), 1, outputImage);

        for (i = 1; i < numProcesses; i++)
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

        for (int i = 0; i < arrayLenght; i++)
        {
            pixel.red = pixels[i].red;
            pixel.green = pixels[i].green;
            pixel.blue = pixels[i].blue;
            fwrite(&pixel, sizeof(PIXEL), 1, outputImage);
        }

        for (int j = 0; j < alignment; j++)
        {
            fread(&aux, sizeof(unsigned char), 1, bmpImage);
            fwrite(&aux, sizeof(unsigned char), 1, outputImage);
        }

        shmdt(pixels);
        shmctl(shmid, IPC_RMID, NULL);
        deallocPixelBitmap(pixelBitmap, rows);
        fclose(bmpImage);
        fclose(outputImage);
    }

    //=======================================

    return 0;
}

FILE *openFile(char filePath[50], char mode[3])
{
    FILE *file = fopen(filePath, mode);
    if (file == NULL)
    {
        error(filePath, NULL);
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
            // fiz uma lógica para achar um ponto valido da máscara no array.
            // pensei numa maneira de simular que estamos navegando uma matriz, sendo que
            // na verdade temos apenas um array.
            for (int maskRow = maskStartingRow, j = 0; j < mask && j < rows; j++, maskRow++)
            {
                for (int maskCol = maskStartingCol, k = 0; k < mask && k < cols; k++, maskCol++)
                {
                    // uso long pq imagens muito grandes estam resultando
                    // em um valor maior que o de um inteiro
                    long arrayIndex = maskRow * cols + maskCol;
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

void mapImageToArray(HEADER *header, PIXEL **pixelBitmap, PIXEL *pixels, FILE *file)
{

    /**
     * Esse método vai mapear os valores dos pixeis para um array de pixeis, assim
     * podemos guardar seus valores em um espaço de memória compartilhado.
    */

    long arrayIndex;
    PIXEL pixel;

    // printFileDetails((*header));

    for (int i = 0; i < header->height; i++)
    {
        for (int j = 0; j < header->width; j++)
        {
            fread(&pixel, sizeof(PIXEL), 1, file);
            arrayIndex = i * header->width + j;
            pixels[arrayIndex].red = pixel.red;
            pixels[arrayIndex].green = pixel.green;
            pixels[arrayIndex].blue = pixel.blue;
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

void error(char filePath[50], char message[100])
{
    printf("Erro ao abrir arquivo %s\n", filePath);
    if (message)
        printf("%s\n", message);
    exit(0);
}
