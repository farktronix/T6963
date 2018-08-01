#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../t6963_commands.h"
#include "bmp.h"

#define DEBUG 0

static char *lcd_path = "/dev/lcd";


// takes a two dimensional array [7][bmp_size]
// first dimension is 7 elements, each holding a second dimension with the bitmap
// shifted by one pixel
void precalc_bitshift(unsigned char *bits, const unsigned char *orig,
        unsigned int rowlen, unsigned int rows) {
    unsigned char mask=0, carry=0, loop=0;
    int i,j,k;
    unsigned char *cur;
    const unsigned char *prev;

    prev=orig;
    cur=bits;

    for(i=0;i<7;i++) {
        mask|=0x80;
        cur=bits+i*rowlen*rows;
        
        for(j=0;j<rows;j++) {
            loop=((*(orig+j*rowlen))&mask)>>(7-i);

            for(k=rowlen-1;k>=0;k--) {
                carry=((*(orig+j*rowlen+k))&mask)>>(7-i);
                *(cur+j*rowlen+k)=(*(orig+j*rowlen+k))<<(i+1);
                *(cur+j*rowlen+k)|=loop;
                loop=carry;
            }
        }
        mask>>=1;
    }
    memcpy(bits+(7*rowlen*rows), orig, rowlen*rows);
}

int main(int argc, char *argv[]) {
    int lcd;

    struct bmp_info bmpinfo;

    unsigned char *bmpdata;
    unsigned char *bmp;
    unsigned char *bitshift;

    struct t6963_status lcd_status;

    unsigned int graphics_base_1, graphics_base_2;
    unsigned int *frame_ptr, *buf_ptr, *swp_ptr;
    unsigned char *clear;
    unsigned int rowwid;

    int i, j, k, addr;
    char err;

    unsigned char scroll=0;
    unsigned int delay=10000;

    if(argc<2) {
        printf("usage: %s filename [-s] [-d]\n\t-s\tscroll image"
               "\n\t-d\tdelay between scroll updates in microseconds\n", argv[0]);
        exit(-1);
    }

    if(argc>2) {
        for(i=2;i<argc;i++) {
            if(argv[i][0] == '-' && argv[i][1] == 's')
                scroll=1;
            if(argv[i][0] == '-' && argv[i][1] == 'd')
                delay=atoi(argv[i+1]);
        }
    }
    if(DEBUG) 
        printf("delay: %d\n", delay);

    if((lcd=open(lcd_path, O_RDWR))<0) {
        perror("could not open LCD device");
        exit(-1);
    }

    // poke the LCD
    ioctl(lcd, T6963_GET_STATUS, &lcd_status);
    ioctl(lcd, T6963_CLEAR_GRAPHICS, 0);

    // init framebuffer
    graphics_base_1=lcd_status.graphics_base;
    graphics_base_2=lcd_status.graphics_base+(8*lcd_status.row_width*lcd_status.rows);
    frame_ptr=&graphics_base_1;
    buf_ptr=&graphics_base_2;
    clear=(unsigned char*)malloc(8*lcd_status.row_width*lcd_status.rows);
    memset(clear, 0x00, 8*lcd_status.row_width*lcd_status.rows);

    if(DEBUG)
        printf("graphics base: 0x%04x row_width: 0x%04x\n", 
            lcd_status.graphics_base, lcd_status.row_width);

    ioctl(lcd, T6963_ADDR, frame_ptr); //set address to graphics mem base

    bmpdata=bmp_loadfile(argv[1], &bmpinfo, &err);
    if(err<0) {
        printf("could not load bmp file! %d\n", err);
        exit(-1);
    }

    //clip to size of bmp
    rowwid=bmpinfo.width<lcd_status.row_width?bmpinfo.width:lcd_status.row_width; 

    bmp=bmpdata;

    if(!scroll) {
        for(i=0;i<8*lcd_status.rows;i++) 
            write(lcd, bmp+(i*bmpinfo.width), rowwid);
        exit(0);
    }

    // allocate and fill single bit scrolling precalc buffer
    bitshift=(unsigned char*)malloc(8*bmpinfo.height*bmpinfo.width);
    precalc_bitshift(bitshift, bmp, bmpinfo.width, bmpinfo.height);

    bmp=bitshift;
   
    // let's get this thing rolling...
    j=0;
    while(1) {
        for(k=0;k<8;k++) {
            for(i=0;i<8*lcd_status.rows;i++) {
                // jump from row to row
                addr=*buf_ptr+(i*lcd_status.row_width);
                ioctl(lcd, T6963_ADDR, &addr);
               
                // correct alignment on wraparound
                if(j+rowwid>bmpinfo.width) {
                    write(lcd, bmp+(i*bmpinfo.width)+j, bmpinfo.width-j);
                    write(lcd, bmp+(i*bmpinfo.width), rowwid-(bmpinfo.width-j));
                } else {
                    write(lcd, bmp+(i*bmpinfo.width)+j, rowwid);
                }
            }
            // swap framebuffers
            swp_ptr=frame_ptr;
            frame_ptr=buf_ptr;
            buf_ptr=swp_ptr;
            ioctl(lcd, T6963_SET_GRAPHICS_BASE, frame_ptr);
            
            // shift down one bit by selecting the next array
            bmp=bitshift+(k*bmpinfo.width*bmpinfo.height);
            usleep(delay);
        }
        // we've shifted a whole byte
        j=(j+1)%bmpinfo.width;
    }

    return 0;
}
