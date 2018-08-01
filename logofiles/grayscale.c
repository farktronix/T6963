#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "../t6963_commands.h"
#include "bmp.h"

#define DEBUG           1

#define DUMP_BUFFERS    1

static char *lcd_path = "/dev/lcd";

// turns on the pixel "at" bits from the beginning of *buf
void pixel_on(unsigned long at, unsigned char *buf) {
    unsigned int base_addr=at/8;
    unsigned char mask=0x80;
    mask>>=(at%8);
    *(buf+base_addr)|=mask;
}

// only takes 8 bit bitmaps right now
// buffers should be 1/8 the size of orig and there should be num_buffers many of them
void createBuffers(unsigned char *buffers, const unsigned char *orig, 
        unsigned int rowlen, unsigned int rows, unsigned char num_buffers) {
    unsigned long i;
    int j;
    unsigned int bufstep=255/num_buffers;

    for(i=0;i<rowlen*8*rows;i++) {
        for(j=1;j<=num_buffers;j++) {
            if(orig[i]<=j*bufstep) {
                pixel_on(i, buffers+(j-1)*rowlen*rows);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int lcd;
    int i, j;

    struct bmp_info bmpinfo;
    struct t6963_status lcd_status;

    // buffer addresses in LCD memory
    unsigned int *buffers;
    unsigned int buf_addr;

    // bitmap buffers
    unsigned char *bmpdata;
    unsigned char *colorBuf;
    unsigned char *buf_ptr;

    char err;
    unsigned int rowwid;
    unsigned int addr;
    unsigned char num_buffers=6;
    unsigned long delay=10000;

#ifdef DUMP_BUFFERS
    unsigned char dump_hdr[62];
    unsigned char buf_name="bufferx";
    int buf_file;
    unsigned long buf_height;
#endif

    if(argc<2) {
        printf("usage: %s filename [-b buffers] [-d delay]\n", argv[0]);
        exit(-1);
    }

    if(argc>2) {
        for(i=2;i<argc;i++) {
            if(argv[i][0]=='-' && argv[i][1]=='b') {
                num_buffers=atoi(argv[i+1]);
            }
            if(argv[i][0]=='-' && argv[i][1]=='d') {
                delay=atoi(argv[i+1]);
            }
        }
    }

    printf("displaying %s to screen with %d levels of grayscale, %.6f seconds "
            "between refresh\n", argv[1], num_buffers, delay/1000000.0);

#ifndef DUMP_BUFFERS
    if((lcd=open(lcd_path, O_RDWR))<0) {
        perror("could not open LCD device");
        exit(-1);
    }

    // poke the LCD
    ioctl(lcd, T6963_GET_STATUS, &lcd_status);
    ioctl(lcd, T6963_CLEAR_GRAPHICS, 0);
#endif

    // load the bitmap
    bmpdata=bmp_loadfile(argv[1], &bmpinfo, &err);
    if(err<0) {
        perror("could not load bmp file!");
        exit(-1);
    }

    // set up buffer addresses on LCD
    for(i=0;i<num_buffers;i++)
        buffers[i]=lcd_status.graphics_base+(i*bmpinfo.width*bmpinfo.height);
    
    // init buffers
    colorBuf=(unsigned char*)malloc(bmpinfo.width*bmpinfo.height*num_buffers);
    memset(colorBuf, 0x00, bmpinfo.width*bmpinfo.height*num_buffers);
    createBuffers(colorBuf, bmpdata, bmpinfo.width, bmpinfo.height, num_buffers);

#ifdef DUMP_BUFFERS
    memset(dump_hdr, 0x00, 62);
    dump_hdr[0]=0x4d;
    dump_hdr[1]=0x42;
    dump_hdr[2]=0x3e;
    dump_hdr[3]=0x05;
    dump_hdr[4]=0;
    dump_hdr[5]=0;
    dump_hdr[10]=0x3e;
    dump_hdr[18]=0x20;
    
    buf_height=-1-(0x20);
    dump_hdr[22]=buf_height;
    dump_hdr[23]=(buf_height&0x0000ff00)>>8;
    dump_hdr[24]=(buf_height&0x00ff0000)>>16;
    dump_hdr[25]=(buf_height&0xff000000)>>24;
    dump_hdr[29]=1;
    
    for(i=0;i<num_buffers;i++) {
        snprintf(buf_name, 8, "buffer%1d", i);
        buf_file=open(buf_name, O_RDWR | O_CREAT, 0);
        write(buf_file, dump_hdr, 62);
        write(buf_file, colorBuf+(bmpinfo.width*bmpinfo.height*i), bmpinfo.width*bmpinfo.height);
        close(buf_name);
    }

    return 0;
#endif

    //clip to size of bmp
    rowwid=bmpinfo.width<lcd_status.row_width?bmpinfo.width:lcd_status.row_width; 

    // write buffers to display 
    for(j=0;j<num_buffers;j++) {
        buf_ptr=colorBuf+(bmpinfo.width*bmpinfo.height*j);
        buf_addr=*(buffers+j);

        for(i=0;i<8*lcd_status.rows;i++) {
            // jump from row to row
            addr=(buf_addr)+(i*lcd_status.row_width);
            ioctl(lcd, T6963_ADDR, &addr);
           
            write(lcd, buf_ptr+(i*bmpinfo.width), rowwid);
        }
    }

    
    while(1) {
        for(i=0;i<num_buffers;i++) {
            ioctl(lcd, T6963_SET_GRAPHICS_BASE, buffers+i);
            usleep(delay);
        }
    }
    
    return 0;
}
