#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <signal.h>

void work(int begin, int end, float t, unsigned char *bmpa, unsigned char *bmpb, unsigned char *out){
    for(int s = begin; s < end; s++){
        out[s] = bmpa[s] * t + bmpb[s] * (1-t); 
    }
}


int main(int argc, char *argv[]){
    struct  timespec start, finish;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int processes = atoi(argv[1]);
    float mixFactor = atof(argv[2]);
    FILE *BMPA = fopen(argv[3], "rb");
    FILE *BMPB = fopen(argv[4], "rb");

    if(mixFactor < 0) mixFactor = 0; //image goes rave(y) otherwise
    else if(mixFactor > 1) mixFactor = 1;        

    FILE *outBMP = fopen("out.bmp", "wb");
        
    unsigned char fileHeader[14];
    fread(fileHeader, 1, 14, BMPA);
    fread(fileHeader, 1, 14, BMPB);
    fwrite(&fileHeader, 1, 14, outBMP);

    unsigned char imageHeader[40];
    fread(imageHeader, 1, 40, BMPA);
    fread(imageHeader, 1, 40, BMPB);
    fwrite(&imageHeader, 1, 40, outBMP);

    int width = ( (imageHeader[4]) + (imageHeader[5] << 8) + (imageHeader[6] << 16) + (imageHeader[7] << 24) );
    int height = ( (imageHeader[8]) + (imageHeader[9] << 8) + (imageHeader[10] << 16) + (imageHeader[11] << 24) );
    
    int paddingNeeded = 0;
    if((width * 3) % 4 != 0){
        paddingNeeded = 4 - ((width * 3) % 4);
    }
    int paddedLen = ((width * height * 3) + (paddingNeeded * height));
    
    unsigned char *bmpaData = mmap(NULL, paddedLen, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    unsigned char *bmpbData = mmap(NULL, paddedLen, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    unsigned char *outData = mmap(NULL, paddedLen, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fread(bmpaData, 1, paddedLen, BMPA);
    fread(bmpbData, 1, paddedLen, BMPB);
    fclose(BMPA);
    fclose(BMPB);


    // access them from work
    //mmaped work to the outfile


    int offset = paddedLen/processes;
    int begin = 0;
    int end = offset;
    for(int i = 0; i < processes; i ++){
        int pid = fork();

        if(pid == 0){
            work(begin, end, mixFactor, bmpaData, bmpbData, outData);
            _exit(0);
        }
        begin = end;
        end = end + offset; 
    }


    if(paddedLen % processes != 0){
        work((paddedLen - (paddedLen % processes)), paddedLen, mixFactor, bmpaData, bmpbData, outData);
    }

    for(int i = 0; i < processes; i++){
        wait(0);
    }


    fwrite(outData, 1, paddedLen, outBMP);
    fclose(outBMP);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    printf("Run time: %f\n", (double) ( (finish.tv_sec - start.tv_sec) + ( (finish.tv_nsec - start.tv_nsec) / 1e9) )  );
}
        




