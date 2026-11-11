#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>   
#include <sys/stat.h>  
#include <sys/wait.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

//apply horizontal blur
//apply veritcal blur
//radius of 4 pixels
//atomic interger counter to distribute the row and columb passes among threads
//synchronize threads between passes (barrier)
//compare execution time to single thread


// w0 + 2(w1 + w2 + w3 + w4) weights should be normalized for each dimension

//  ./blur [input image] [output image] [blurfactor]

//blurfacotr -- how many times the two step process should be repeated.

atomic_int atomicRow = 0;
atomic_int atomicCol = 0;
pthread_barrier_t blur_barrier;

static float w[5] = {0.204164f, 0.180174f, 0.123832f, 0.066283f, 0.027631f};

struct workerData{
    int width;
    int height;
    int rowSize;
    int blurFactor;
    int paddingNeeded;
    unsigned char *imageData;
    unsigned char *outData;
};


//bonus? - depth map
int blurH(int i, unsigned char *imageData){
    return ( ( (imageData[i-12] + imageData[i+12] ) * w[4] ) + 
    ( (imageData[i-9] + imageData[i+9] ) * w[3] ) +
    ( (imageData[i-6] + imageData[i+6] ) * w[2] ) +
    ( (imageData[i-3] + imageData[i+3] ) * w[1] ) +
    ( imageData[i] * w[0]) );

}

int blurV(int i, int rowsize, int h, unsigned char *imageData){
        return ( ( (imageData[i - (rowsize * 4)] + imageData[i + (rowsize * 4)] ) * w[4] ) + 
    ( (imageData[i-(rowsize * 3)] + imageData[i+(rowsize * 3)] ) * w[3] ) +
    ( (imageData[i-(rowsize * 2)] + imageData[i+(rowsize * 2)] ) * w[2] ) +
    ( (imageData[i-(rowsize * 1)] + imageData[i+(rowsize * 1)] ) * w[1] ) +
    ( imageData[i] * w[0]) );
}

void *worker(void *arg) {

    struct workerData *data = (struct workerData*)arg; 
    
    for(int times = 0; times < data->blurFactor; times++){

        //horizontal blur;
        while(1){
            int h = atomic_fetch_add(&atomicRow, 1);
            if(h >= data->height){break;}

            for(int w = 0; w < (data->width * 3); w++){

                if(w <= 12 || w >= (data->width - 4) * 3){
                    data->outData[w + (h * data->rowSize)] = data->imageData[w + (h * data->rowSize)];
                    continue;
                }

                data->outData[w + (h * data->rowSize)] = blurH(w + (h * data->rowSize), data->imageData);   
            }
            
            for(int i = 0; i < data->paddingNeeded; i++){
                data->outData[ ( (data->width * 3) + (h * data->rowSize)) + i] = 0;
            }
        }

        pthread_barrier_wait(&blur_barrier);

        //vertical blur
        while(1){
            int w = atomic_fetch_add(&atomicCol, 1);
            if(w >= data->rowSize){break;} 

            for(int h = 0; h < data->height; h++){
                if(h <= 4 || (data->height - h) <= 4){
                    data->imageData[w + (h * data->rowSize)] = data->outData[w + (h * data->rowSize)];
                    continue;
                }
                if(w >= (data->width * 3)){
                    data->imageData[(data->rowSize * h) + w] = 0;
                    continue;
                }

                data->imageData[w + (h * data->rowSize)] = blurV(w + ( h * data->rowSize), data->rowSize, h, data->outData);
            }
        }

        pthread_barrier_wait(&blur_barrier);
        
        
        atomic_store(&atomicRow, 0);
        atomic_store(&atomicCol, 0);
 

        pthread_barrier_wait(&blur_barrier);
    
    }

}






int main(int argc, char *argv[]){
    struct  timespec start, finish, startThread, finishThread;
        
    FILE *inBMP = fopen(argv[1], "rb");
    FILE *outBMP = fopen(argv[2], "wb");
    int blurFactor = atoi(argv[3]);

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
    int columnSize = (height * 3);

    unsigned char *imageData = malloc(padded);
    unsigned char *outData = malloc(padded); 
    fread(imageData, 1, padded, inBMP);

/*SINGLE THREAD EXECUTION FOR TIME KEEPING*/
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(int times = 0; times < blurFactor; times++){
        //horizontal blur;
        for(int h = 0; h < height; h++){
            for(int w = 0; w < (width * 3); w++){
                if(w <= 12 || w >= (width - 4) * 3){
                    outData[w + (h * rowSize)] = imageData[w + (h * rowSize)];
                    continue;
                }

                outData[w + (h * rowSize)] = blurH(w + (h * rowSize), imageData);   
            }
            
            for(int i = 0; i < paddingNeeded; i++){
                outData[ ( (width * 3) + (h * rowSize)) + i] = 0;
            }

        }

        //vertical blur
        for(int w = 0; w < (width * 3) + paddingNeeded; w++){
            for(int h = 0; h < height; h++){
                if(h <= 4 || (height - h) <= 4){
                    imageData[w + (h * rowSize)] = outData[w + (h * rowSize)];
                    continue;
                }
                if(w >= (width * 3)){
                    imageData[(rowSize * h) + w] = 0;
                    continue;
                }

                imageData[w + (h * rowSize)] = blurV(w + ( h * rowSize), rowSize, h, outData);
            }
        }

    }
/*SINGLE THREAD DONE*/
    clock_gettime(CLOCK_MONOTONIC, &finish);
    
    free(imageData);
    free(outData);
    imageData = malloc(padded);
    outData = malloc(padded); 
    fclose(inBMP);
    inBMP = fopen(argv[1], "rb");
    fread(fileHeader, 1, 14, inBMP);
    fread(imageHeader, 1, 40, inBMP);
    fread(imageData, 1, padded, inBMP);

/*MULTI THREAD EXECUTION FOR TIME KEEPING*/

    pthread_barrier_init(&blur_barrier, NULL, 4);

    struct workerData threadStuff = {
        .width = width,
        .height = height,
        .rowSize = rowSize,
        .blurFactor = blurFactor,
        .paddingNeeded = paddingNeeded,
        .imageData = imageData,
        .outData = outData
    };

    pthread_t threads[4];

    clock_gettime(CLOCK_MONOTONIC, &startThread);

    for(int i = 0; i < 4; i++){
        pthread_create(&threads[i], NULL, worker, &threadStuff);
    }
    
    for(int i = 0; i < 4; i++){
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &finishThread);

    
    pthread_barrier_destroy(&blur_barrier);

    fwrite(imageData, 1, padded, outBMP);
    fclose(inBMP);
    fclose(outBMP);
    free(imageData);
    free(outData);

    printf("****************FINISHED****************\n");
    printf("Single thread run time: %f seconds\n", (double) ( (finish.tv_sec - start.tv_sec) + ( (finish.tv_nsec - start.tv_nsec) / 1e9) )  );
    printf("Multi thread run time: %f seconds\n", (double) ( (finishThread.tv_sec - startThread.tv_sec) + ( (finishThread.tv_nsec - startThread.tv_nsec) / 1e9) )  );

}



