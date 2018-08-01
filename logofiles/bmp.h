#ifndef __BMP_H
#define __BMP_H

struct bmp_info {
    unsigned int type;   // if it's 'BM' you're in luck
    unsigned long fsize; // size of entire bmp file
    unsigned int zero1;
    unsigned int zero2;
    unsigned long offset; // offset from start of file to start of data
    unsigned long size; // size of bmp header info

    unsigned long width;  // width in bytes, aligned to 4 bytes
    unsigned long height; // height in bits
    unsigned int  planes;
    unsigned int  bits;
    unsigned long compression;
    unsigned long imagesize;
    unsigned long xPxPerM;
    unsigned long yPxPerM;
    unsigned long colors;
    unsigned long importantColors;
};

unsigned char *bmp_loadfile(const char *filename, struct bmp_info *bmpinfo, char *err);

#endif
