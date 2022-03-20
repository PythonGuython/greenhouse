


#ifndef __TIFFSTUFF_H
#define __TIFFSTUFF_H



int tiffwrite (const char* fname, char* img, int width, int height, int bps, char* comment);

int pnm_write_8 (char* fname, unsigned char* img, int width, int height);
int pnm_write_16 (char* fname, short* img, int width, int height);
int pnm_write_rgb (char* fname, unsigned char* img, int width, int height);


#endif

