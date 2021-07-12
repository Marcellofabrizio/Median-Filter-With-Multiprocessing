#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>

typedef struct pixel
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} PIXEL;

typedef struct arguments
{
    PIXEL *imageArray;
    int rows;
    int cols;
    int sequential;
    int numInstances;
    int mask;
} ARGUMENTS;

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
void *medianFilter_args(void *args);
void medianFilter(PIXEL *imageArray, int rows, int cols, int sequential, int numProcesses, int mask);
void mapImageToArray(HEADER *header, PIXEL **pixelBitmap, PIXEL *pixels, FILE *file);
void error(char filePath[50], char message[100]);
void writeImage(HEADER *bmpHeader, PIXEL *pixels, FILE *file, int numInstances, char outputFilePath[100], int cols, int arrayLength);
// =================================================================================================

#endif