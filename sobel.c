//name shared memory
//2 processes
//wait for memory to be created
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>   
#include <sys/stat.h>  
#include <sys/wait.h>
#include <math.h>
#include <string.h>

void sobel(unsigned char *imageData, unsigned char *sobelImage, int width, int height, int rowSize, int startRow, int endRow){

    for(int y = startRow; y < endRow; y++){
        for(int x = 1; x < width - 1; x++){
            int topLeft = (imageData[(y - 1) * rowSize + (x - 1) * 3] + imageData[(y - 1) * rowSize + (x - 1) * 3 + 1] + imageData[(y - 1) * rowSize + (x - 1) * 3 + 2]) / 3;

            int top = (imageData[(y - 1) * rowSize + x * 3] + imageData[(y - 1) * rowSize + x * 3 + 1] + imageData[(y - 1) * rowSize + x * 3 + 2]) / 3;

            int topRight = (imageData[(y - 1) * rowSize + (x + 1) * 3] + imageData[(y - 1) * rowSize + (x + 1) * 3 + 1] + imageData[(y - 1) * rowSize + (x + 1) * 3 + 2]) / 3;

            int middleLeft  = (imageData[y * rowSize + (x - 1) * 3] + imageData[y * rowSize + (x - 1) * 3 + 1] + imageData[y * rowSize + (x - 1) * 3 + 2]) / 3;

            int middleRight = (imageData[y * rowSize + (x + 1) * 3] + imageData[y * rowSize + (x + 1) * 3 + 1] + imageData[y * rowSize + (x + 1) * 3 + 2]) / 3;

            int bottomLeft  = (imageData[(y + 1) * rowSize + (x - 1) * 3] + imageData[(y + 1) * rowSize + (x - 1) * 3 + 1] + imageData[(y + 1) * rowSize + (x - 1) * 3 + 2]) / 3;

            int bottom = (imageData[(y + 1) * rowSize + x * 3] +  imageData[(y + 1) * rowSize + x * 3 + 1] + imageData[(y + 1) * rowSize + x * 3 + 2]) / 3;

            int bottomRight = (imageData[(y + 1) * rowSize + (x + 1) * 3] + imageData[(y + 1) * rowSize + (x + 1) * 3 + 1] + imageData[(y + 1) * rowSize + (x + 1) * 3 + 2]) / 3;


            
            int gx = -topLeft + topRight - 2 * middleLeft + 2 * middleRight - bottomLeft + bottomRight;
            int gy = -topLeft - 2 * top - topRight + bottomLeft + 2 * bottom + bottomRight;
            int edge = (int)sqrt(gx * gx + gy * gy);

            edge = (edge > 255) ? 255 : edge;

            int output = y * rowSize + x * 3;

            sobelImage[output] = edge;       // Blue
            sobelImage[output + 1] = edge;   // Green
            sobelImage[output + 2] = edge;   // Red
        }
    }
}
 

int main(){

    FILE *inBMP = fopen("img.bmp", "rb");
    FILE *outBMP = fopen("sobel.bmp", "wb");

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

    int midY = height/2;
    
    shm_unlink("/FirstTry");
    int id = fork();
    if(id == 0){
        int fd = -1;
        while(fd < 0){
            fd = shm_open("/FirstTry", O_RDWR, 0777);
            usleep(10000);
        }

        unsigned char *data = mmap(0, padded * 2 + sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        unsigned char *imageData = data;
        unsigned char *sobelImage = data + padded;

        sobel(imageData, sobelImage, width, height, rowSize, midY, height - 1);

        _exit(0);
    }


    int fd = shm_open("/FirstTry", O_CREAT | O_RDWR, 0777);
    ftruncate(fd, padded * 2 + sizeof(int));

    unsigned char *data = mmap(0, padded * 2 + sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    unsigned char *imageData = data;
    unsigned char *sobelImage = data + padded;
    
    fread(imageData, 1, padded, inBMP);
    fclose(inBMP);

    sobel(imageData, sobelImage, width, height, rowSize, 1, midY);



    wait(0);
    fwrite(sobelImage, 1, padded, outBMP);
    fclose(outBMP);
    munmap(data, padded * 2 + sizeof(int));
    close(fd);
    shm_unlink("/FirstTry");

}


