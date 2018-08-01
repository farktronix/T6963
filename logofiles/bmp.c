#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bmp.h"

#define DEBUG 0

unsigned char *bmp_loadfile(const char *filename, struct bmp_info *bmpinfo, char *err) {
    int file;
    unsigned char bmpdata[55];
    struct stat bmpstat;
    int bmp_pad, bmprow_w;
    unsigned char bmp_mask;
    unsigned char *data;
    int i;

    *err=0;

    if((file=open(filename, O_RDONLY))<0) {
        printf("could not open file");
        *err=-1;
        return 0;
    }

    if(fstat(file, &bmpstat)<0) {
        printf("stat of bmp file");
        *err=-2;
        return 0;
    }

    if(DEBUG)
        printf("loading %li byte bitmap...\n", bmpstat.st_size);

    if(read(file, bmpdata, sizeof(bmpdata))<0) {
        printf("could not read bmp header");
        *err=-3;
        return 0;
    }

    bmpinfo->type=bmpdata[0]<<8|bmpdata[1];
    bmpinfo->fsize=bmpdata[5]<<24|bmpdata[4]<<16|bmpdata[3]<<8|bmpdata[2];
    bmpinfo->zero1=bmpdata[6]<<8|bmpdata[7];
    bmpinfo->zero2=bmpdata[8]<<8|bmpdata[9];
    bmpinfo->offset=bmpdata[13]<<24|bmpdata[12]<<16|bmpdata[11]<<8|bmpdata[10];

    if(bmpinfo->type!=0x424d) {
        printf("invalid bmp type");
        *err=-4;
        return 0;
    }

    // we don't care about the rest of the fields for now
    bmpinfo->size=bmpdata[17]<<24|bmpdata[16]<<16|bmpdata[15]<<8|bmpdata[14];
    bmpinfo->width=bmpdata[21]<<24|bmpdata[20]<<16|bmpdata[19]<<8|bmpdata[18];
    bmpinfo->height=-1-(bmpdata[25]<<24|bmpdata[24]<<16|bmpdata[23]<<8|bmpdata[22])+1;
    bmpinfo->bits=bmpdata[30]<<8|bmpdata[29];

    data=(unsigned char*)malloc(bmpstat.st_size);
    if(data==NULL) {
        printf("error: out of memory!\n");
        *err=-5;
        return 0;
    }

    if(lseek(file, bmpinfo->offset, SEEK_SET)<0) {
        printf("could not seek bmp file");
        *err=-6;
        return 0;
    }

    if(read(file, data, bmpstat.st_size)<0) {
        printf("could not read bmp file");
        *err=-7;
        return 0;
    }

    // align bmp on 4 byte boundary and remove crap
    bmp_pad=bmpinfo->width;
    bmpinfo->width/=8;
    bmprow_w=bmpinfo->width%4?bmpinfo->width+(4-(bmpinfo->width%4)):bmpinfo->width;

    bmp_pad=(bmprow_w*8)-bmp_pad;
    if(bmp_pad) {
        for(i=0;i<bmp_pad;i++) {
            bmp_mask<<=1;
            bmp_mask|=0x01;
        }
        bmp_mask=~bmp_mask;
        for(i=0;i<bmpinfo->height;i++) {
            *(data+i*bmprow_w-1)&=bmp_mask;
        }
    }

    bmpinfo->width=bmprow_w;

    return data;
}
