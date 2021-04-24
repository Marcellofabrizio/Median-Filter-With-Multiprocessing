#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <stdint.h>

using namespace std;

#pragma pack(push, 1) /* diz pro compilador não alterar alinhamento
                         ou tamanho da struct */

struct pixel{uint8_t r,g,b;};

struct bmpHeader {

    uint16_t type;

    uint32_t fileSize;

    uint16_t reserved1, reserved2;

    uint32_t offset, header_size;

    int32_t width, height;

    uint16_t planes, bits;

    uint32_t compressionType, imagesize;

    int32_t xresolution, yresolution;

    uint32_t ncolours, importantcolours;

    uint32_t redbitmask,greenbitmask,bluebitmask,alphabitmask;

    uint32_t ColorSpaceType;

    uint32_t ColorSpaceEndPoints[9];

    uint32_t Gamma_Red,Gamma_Green,Gamma_Blue,intent,ICCProfileData,ICCProfileSize,Reserved;

};

unsigned char SCALELIST[10] = {' ','.',':','-','=','+','*','#','%','@'};
string scaleString = " .,:;ox%#@";


void printFileDetails(bmpHeader *bH) 
{
    printf("File Size is %d bytes\n", bH->fileSize);
    printf("Bits per pixel is %hu\n", bH->bits);
    printf("X resolution: %d - Y resolution: %d (ps: pixels per meter)\n", bH->xresolution,
                                                                           bH->yresolution);
    printf("Header size is %d\n", bH->header_size);
}

void readBmpFile(bmpHeader *header, pixel **bitmap, FILE *bmp_image) 
{
    
    for (int i = 0; i < header->height; i++)
    {
        bitmap[i] = (struct pixel*) malloc(sizeof(struct pixel) * header->width);
    }

    for (int i = 0; i < header->height; i++)
    {
        for (int j = 0; j < header->width; j++)
        {
            fread(&bitmap[i][j], sizeof(struct pixel), 1, bmp_image);
        }
    }

}

void writeTxtFile(bmpHeader *header, pixel **bitmap, FILE *ascii_image)
{
    int i, j;
    
    printf("header height: %d\n", header->height);

    for (i = header->height - 1; i >= 0; i--)
    //bmp file is little endian, so it needs to be writen
    //from end to start
    {
        for (j = 0; j < header->width; j++)
        {
            //for greyscale, found this out more than
            //a year after I did this
            int average = ((bitmap[i][j].r*0.2126) 
                         + (bitmap[i][j].g*0.7152) 
                         + (bitmap[i][j].b*0.0722))/3;
            
            average = average/10; 
            fwrite(&SCALELIST[average], sizeof(char), 1, ascii_image);
        }
        char linebreak = '\n';
        fwrite(&linebreak, sizeof(char), 1, ascii_image);
    }

    for (i = 0; i < header->height; i++)
    {
        for (j = 0; j < header->width; j++)
        {
            fwrite(&bitmap[i][j], sizeof(struct pixel), 1, ascii_image);
        }
    }

}

int main() {
    FILE *bitmap_image;
    FILE *ascii_image;
    bitmap_image = fopen("./files/images/person.bmp", "rb");
    ascii_image = fopen("./files/output_files/person.txt", "wt");

    struct bmpHeader bH;

    if (bitmap_image == NULL)
        perror("Error opening image\n");


    fread(&bH, sizeof(struct bmpHeader), 1, bitmap_image);
    fwrite(&bH, sizeof(struct bmpHeader), 1, ascii_image);

    struct pixel **bitmap;

    bitmap = (struct pixel **) malloc(sizeof(struct pixel *) * bH.height);

    readBmpFile(&bH, bitmap, bitmap_image);
    writeTxtFile(&bH, bitmap, ascii_image); // causes core dump 

    for (int i = 0; i < bH.height; i++) {
        free(bitmap[i]);
    }

    free(bitmap);

    printFileDetails(&bH);

    fclose(bitmap_image);
    fclose(ascii_image);

    return 0;
}

