#include "median_filter.h"

#define T //PARA THREADS
#define P //PARA PROCESSOS

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#pragma pack(push, 1) /* diz pro compilador não alterar alinhamento \
                         ou tamanho da struct */

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
    if (argc != 4)
    {
        printf("<NumberOfInstances> <MaskSize> <InputFilePath>\n");
        exit(0);
    }

    int numInstances, mask;
    numInstances = atoi(argv[1]);
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

    pthread_t *t_id = NULL;
    ARGUMENTS *arguments = NULL;

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

    printf("%d", bmpHeader.fileSize);
    // printFileDetails(bmpHeader);
    //=========  MULTIPROCESSING =========

#ifdef P

    shmid = shmget(key, sizeof(PIXEL) * arrayLenght, IPC_CREAT | 0644);
    pixels = (PIXEL *)shmat(shmid, NULL, 0);
    mapImageToArray(&bmpHeader, pixelBitmap, pixels, bmpImage);

    // seq = 0;

    // for (i = 1; i < numInstances; i++)
    // {
    //     pid = fork();
    //     if (pid == 0)
    //     { //child
    //         seq = i;
    //         break;
    //     }
    // }

    // medianFilter(pixels, rows, cols, seq, numInstances, mask);

    // if (seq != 0) //se for filho
    // {
    //     shmdt(pixels);
    // }
    // else if (seq == 0) //se for pai
    // {
    //     writeImage(&bmpHeader, pixels, bmpImage, numInstances, outputFilePath, cols, arrayLenght);

    //     shmdt(pixels);
    //     shmctl(shmid, IPC_RMID, NULL);
    //     deallocPixelBitmap(pixelBitmap, rows);
    // }

#endif

#ifdef T

#endif
}