#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>

void work (int midX, int midY, int width, int height, float rotatingAngle, unsigned char *rotatedImage, unsigned char *imageData, int widthOffset, int FullWidth, int rowSize){
    int x_r; 
    int y_r;

    
    for(int y = 0 ; y < height; y ++){
        for(int x = widthOffset ; x < width; x ++){
            x_r = x - midX;
            y_r = y - midY;

            
            int x_o = (int) ( (cos(-rotatingAngle) * (x_r) ) - (sin(-rotatingAngle) * y_r) ) + midX;
            int y_o = (int) ( (sin(-rotatingAngle) * x_r) + (cos(-rotatingAngle) * (y_r)) ) + midY;


            for(int i = 0; i < 3; i++){
                if((x_o < FullWidth && x_o >= 0) && (y_o < height && y_o >= 0))
                    rotatedImage[( (rowSize * y + x * 3)) + i] = imageData[( (y_o * rowSize + x_o * 3) ) + i];
            }
        }
    }

    _exit(EXIT_SUCCESS);
}


int main (int argc, char *argv[]){
    struct  timespec start, finish;

    clock_gettime(CLOCK_MONOTONIC, &start);

    
    FILE *inBMP = fopen(argv[1], "rb");
    float rotatingAngle = atof(argv[2]);
    int processCount = atoi(argv[3]);
    FILE *outBMP = fopen(argv[4], "wb");




    //:snore: on the structs
    unsigned char fileHeader[14];
    fread(fileHeader, 1, 14, inBMP);
    fwrite(&fileHeader, 1, 14, outBMP);

    unsigned char imageHeader[40];
    fread(imageHeader, 1, 40, inBMP);
    fwrite(&imageHeader, 1, 40, outBMP);

    int width = ( (imageHeader[4]) + (imageHeader[5] << 8) + (imageHeader[6] << 16) + (imageHeader[7] << 24) );
    int height = ( (imageHeader[8]) + (imageHeader[9] << 8) + (imageHeader[10] << 16) + (imageHeader[11] << 24) );
    
    //bitmap width padding
    int paddingNeeded = 0;
    if((width * 3) % 4 != 0 )
        paddingNeeded = (4 - ( (width * 3) % 4) );
    int padded = (width * height * 3) + (paddingNeeded * height);
    int rowSize = width * 3 + paddingNeeded;

    unsigned char *imageData = mmap(NULL, padded, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    unsigned char *rotatedImage = mmap(NULL, padded, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fread(imageData, 1, padded, inBMP);


    int midX = width/2;
    int midY = height/2;

    for(int i = 0; i < processCount; i++){
        int offset = (width * i) / processCount;
        int widthWOffset = (width * (i + 1)) /processCount;

        int id = fork();

        if(id == 0)
            work(midX, midY, widthWOffset, height, rotatingAngle, rotatedImage, imageData, offset, width, rowSize);

    }

    for(int i = 0; i < processCount; i++){
        wait(0);
    }
    

    fwrite(rotatedImage, 1, padded, outBMP);
    fclose(inBMP);
    fclose(outBMP);

    clock_gettime(CLOCK_MONOTONIC, &finish);

    printf("Run time: %f\n", (double) ( (finish.tv_sec - start.tv_sec) + ( (finish.tv_nsec - start.tv_nsec) / 1e9) )  );
}
