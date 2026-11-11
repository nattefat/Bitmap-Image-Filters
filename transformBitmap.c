
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int main (int argc, char *argv[]){

    FILE *inBMP = fopen(argv[1], "rb");
    FILE *outBMP = fopen(argv[2], "wb");
    float changeFactor = atof(argv[4]);

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

    unsigned char *imageData = malloc(padded);
    fread(imageData, 1, padded, inBMP);


    if(argv[3][0] == 'c'){
        int result;
        unsigned char out;

        for(int i = 0; i < (padded); i++){
            result = pow(((double)(imageData[i]))/255, changeFactor) * 255;
            result > 255 ? result = 255 : result;
            out = (unsigned char) result;
            fwrite(&out, 1, 1, outBMP);
        }
    }
        
    else if(argv[3][0] == 's'){
        double b;
        double g;
        double r;
        double avg;
        unsigned char out;

        for(int y = 0 ; y < height; y ++){
            for(int x = 0 ; x < width; x ++){
                b = ((double)(imageData[(( (width * 3 + paddingNeeded) * y) + (x * 3)) ])) / 255;
                g = ((double)(imageData[(( (width * 3 + paddingNeeded) * y) + (x * 3)) + 1])) / 255;
                r = ((double)(imageData[(( (width * 3 + paddingNeeded) * y) + (x * 3)) + 2])) / 255;
                avg = (b + g + r) / 3;

                b = (b + (b-avg) * changeFactor) * 255;
                g = (g + (g-avg) * changeFactor) * 255;
                r = (r + (r-avg) * changeFactor) * 255;

                b > 255 ? b = 255 : (b < 0 ? b = 0 : b);
                g > 255 ? g = 255 : (g < 0 ? g = 0 : g);
                r > 255 ? r = 255 : (r < 0 ? r = 0 : r);

                out = (unsigned char) b;
                fwrite(&out, 1, 1, outBMP);
                out = (unsigned char) g;
                fwrite(&out, 1, 1, outBMP);
                out = (unsigned char) r;
                fwrite(&out, 1, 1, outBMP);
            }
            //out = (unsigned char) 0;
            for(int i = 0; i < paddingNeeded; i ++){
                fwrite(&out, 1, 1, outBMP);
            }

        }
    }

    else if(argv[3][0] == 'l'){
        int result;
        unsigned char out;

        for(int i = 0; i < padded; i++){
            result = ( ((double)imageData[i])/255 + changeFactor) * 255;
            result > 255 ? result = 255 : (result < 0 ? result = 0 : result);
            out = (unsigned char) result;
            fwrite(&out, 1, 1, outBMP); 
        }
    }

    fclose(inBMP);
    fclose(outBMP);
}
