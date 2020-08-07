#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdint.h>
#include "pngwork.h"


static unsigned char paethpredictor(int a, int b, int c){
    int p = a + b - c;
	int pa = abs(p - a);
	int pb = abs(p - b);
	int pc = abs(p - c);
    int pr;

    if (pa <= pb && pa <= pc)
        pr = a;
    else if(pb <= pc)
        pr = b;
    else
        pr = c;
    return (unsigned char)pr;
}

//Takes in chunks of char* returns back as unsigned int.
static unsigned int get32(const unsigned char *const data){
    unsigned char x[4];
    unsigned int u;
    x[0] = data[3];
    x[1] = data[2];
    x[2] = data[1];
    x[3] = data[0];

    memcpy(&u, x, 4);
    return u;
}


int getprocessed_png(char *filename, unsigned *png_width, unsigned *png_height, char **processed_data){
    int filesize;
    FILE *fp = fopen(filename, "rb");
    if(!fp){
        printf("No file input / file input missing\n");
        exit(3);
    }
    fseek(fp, 0, SEEK_END);// goes to end of file
    filesize = ftell(fp);//checks point in file (Which happens to be )

    if (filesize < 58) {
        printf("ER00R!!: PNG too small.\n");
        exit(6);
    }
    
    rewind(fp);
    unsigned char *filedata = malloc(filesize);
    if (!fread(filedata,1,filesize,fp)){ //Puts dada from file into un-named array.
        perror("fread problem");
        exit(2);
    }
    fclose(fp);

    if (memcmp(filedata,"\x89PNG\r\n\x1a\n",9)) {
        printf("ERR0R: missing PNG header\n");
        exit(7);
    }

    process_png(filedata, filesize, png_width, png_height, processed_data);

    return 0;
}


int process_png(unsigned const char *const filedata, int filesize, unsigned *png_width, unsigned *png_height, char **processed_data){



/*loop that seprates data in the image file*/
//    unsigned png_width;   passed into function as pointer
//    unsigned png_height;  passed into function as pointer
    unsigned char png_depth = 0;
    unsigned char png_type = 0; (void)png_type;
    unsigned char png_compression = 0; (void)png_compression;
    unsigned char png_filter = 0; (void)png_filter;
    unsigned char png_interlace = 0; (void)png_interlace;

    unsigned chunksize;
    unsigned chunkname;
    unsigned const char *chunklocation;
    unsigned char *png_byte_data  = NULL;
    unsigned chunkfooter; (void)chunkfooter;
    uLong png_byte_data_size = 0;
    
    int IEND_count = 0;
    int IHDR_count = 0;
    unsigned const char *  /* no: const */  dataindex_pointer = filedata + 8;

    while(dataindex_pointer + 12 <= filedata + filesize){
        chunksize = get32(dataindex_pointer);
        dataindex_pointer += 4;
        //Can check for buffer overflow here.

        chunkname = get32(dataindex_pointer);
        dataindex_pointer += 4;

        chunklocation = dataindex_pointer;//If I did things differently I might not need this
        dataindex_pointer += chunksize;//This line maybe could be after the "if's" 

        chunkfooter = get32(dataindex_pointer); //Can buffer overflow here.
        dataindex_pointer += 4;


        if (chunkname == 0x49444154){ // IDAT
            /*Make space in png_byte_data for new data too be added onto end.*/
            png_byte_data = realloc(png_byte_data,chunksize+png_byte_data_size);
            /*Copies over IDAT chunk data onto free space made by realloc()*/
            memcpy(png_byte_data+png_byte_data_size,chunklocation,chunksize);
            /*Changes recoreded size of png_byte_data to include the new data added to the end by memcpy().*/
            png_byte_data_size += chunksize;
        }
        if (chunkname == 0x49484452){ //IHDR
            *png_width = get32(chunklocation);
            *png_height = get32(chunklocation+4);
            png_depth       = chunklocation[4+4];
            png_type        = chunklocation[4+4+1];
            png_compression = chunklocation[4+4+1+1];
            png_filter      = chunklocation[4+4+1+1+1];
            png_interlace   = chunklocation[4+4+1+1+1+1];
            IHDR_count++;
        }
        if (chunkname == 0x49454e44){ //IEND
            IEND_count++;
        }

    }

    if (IEND_count != 1){
        printf("ERROR: IEND count != 1.\n");
        exit(1);
    }

    if (IHDR_count != 1){
        printf("ERROR: IHDR count != 1.\n");
        exit(1);
    }

    if (!png_byte_data){
        printf("ERROR: No IDAT.\n");
        exit(1);
    }



    unsigned char has_plte; (void)has_plte;
    unsigned char is_rgb; (void)is_rgb;
    unsigned char has_alpha; (void)has_alpha;
    unsigned char channels_per_pixel;

    switch (png_type){
    case 0:
        printf("...Gray Scale\n");
        has_plte = 0;
        is_rgb = 0;
        has_alpha = 0;
        channels_per_pixel = 1;
        break;
    case 2:
        printf("...RGB\n");
        has_plte = 0;
        is_rgb = 1;
        has_alpha = 0;
        channels_per_pixel = 3;
        break;
    case 3:
        printf("...Indexed\n");
        has_plte = 1;
        is_rgb = 1; //kind of true
        has_alpha = 0;
        channels_per_pixel = 1;
        break;
    case 4:
        printf("...Gray Scale and Alpha\n");
        has_plte = 0;
        is_rgb = 0;
        has_alpha = 1;
        channels_per_pixel = 2;
        break;
    case 6:
        printf("...RGB Alpha\n");
        has_plte = 0;
        is_rgb = 1;
        has_alpha = 1;
        channels_per_pixel = 4;
        break;
    default:
        printf("...Something wrong with pngtype...\n");
        printf("%d\n\n",png_type);
        exit(2);
        break;
    }

    
    //bits_per_pixel, filterstride, bytes per row
    unsigned bits_per_pixel = (channels_per_pixel * png_depth);
    unsigned filterstride = bits_per_pixel < 8 ? 1 : bits_per_pixel / 8;
    unsigned pixel_bytesperrow = (*png_width * bits_per_pixel + 7)/8; //pixel_bytesperrow is same in ppm & png
    unsigned total_bytesperrow = pixel_bytesperrow + 1;
    printf("...pixel_bytesperrow: %u\n", pixel_bytesperrow);


    //Undoing zlib and getting ready for taking apart PNG
    printf("...Undoing zlib\n");
    uLong src_size = png_byte_data_size;
    uLong dst_size = total_bytesperrow * *png_height;
    unsigned char *uncompress_png_byte_data = malloc(dst_size);
    int rc = uncompress2(uncompress_png_byte_data,&dst_size,png_byte_data,&src_size);
    if (rc != Z_OK || dst_size != total_bytesperrow * *png_height||src_size != png_byte_data_size){
        printf("Error in: \"Undoing zlib and getting ready for taking apart PNG\"\n");
        printf("det_size: %d\n",(int)dst_size);
        printf("total_bytesperrow: %d\n",total_bytesperrow); //aka png row stride
        printf("*png_height: %d\n",*png_height);
        printf("src_size: %d\n",(int)src_size);
        printf("png_byte_data_size: %d\n",(int)png_byte_data_size);
        exit(2);
    }
    free(png_byte_data);
    
    //ppm image data points to all processed png data
    //it's final size is *png_height*pixel_bytesperrow
    char *ppm_image_data = malloc(*png_height * pixel_bytesperrow);
    *processed_data = ppm_image_data;
    char *ppm_line = ppm_image_data;
    unsigned loopplace = 1;
    unsigned loopplace2 = 0;
    unsigned char *png_line_above = malloc(pixel_bytesperrow + filterstride);
    unsigned char *png_line_current = malloc(pixel_bytesperrow + filterstride);
    unsigned char *png_line = uncompress_png_byte_data;
    unsigned char prediction;


    //Taking apart PNG and making raw image data
    while (loopplace <= *png_height){
        int line_filter_type = *png_line;
        unsigned char *png_line_data_pixel = png_line + 1;
        loopplace += 1;
        loopplace2 = 0;
        memset(png_line_current, 0x00, filterstride); //Makes fake pixels 0x00
        memcpy(png_line_current+filterstride, png_line_data_pixel,pixel_bytesperrow);


        while (loopplace2 < pixel_bytesperrow){
            unsigned char xbyte = *(png_line_current + loopplace2 + filterstride); //current
            unsigned char bbyte = *(png_line_above + loopplace2 + filterstride); //above
            unsigned char cbyte = *(png_line_above + loopplace2); //left & above 
            unsigned char abyte = *(png_line_current + loopplace2); //left
            

            if (line_filter_type == 0x00){
                prediction = 0;
            }else if (line_filter_type == 0x01){
                //sub
                prediction = abyte;
            }else if (line_filter_type == 0x02){
                //up
                prediction = bbyte;
            }else if (line_filter_type == 0x03){
                //average
                prediction = (abyte + bbyte) / 2;
            }else if (line_filter_type == 0x04){
                //paeth
                prediction = paethpredictor(abyte, bbyte, cbyte);
            }else{
                printf("bug325\n");
                exit(325);
            }

            xbyte+=prediction;
            png_line_current[(filterstride + loopplace2)] = xbyte;
            loopplace2 += 1;
            
        }
        
        memcpy(png_line_above,png_line_current,pixel_bytesperrow + filterstride);
        memcpy(ppm_line,png_line_current+filterstride,pixel_bytesperrow);
        png_line += total_bytesperrow;//Moves pointer to next line
        ppm_line += pixel_bytesperrow;//ppm_line is a pointer to an unnamed array
    }
    printf("...Done\n");

	return 0;
}
