/* tiffstuff.c 

	Functions to write an image data array to a TIFF file
	Free bonus offer! Also includes PGM/PBM export.

*/




#include <tiffio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tiffstuff.h"



/*********************************************************************/



/* Write data to a TIFF file. Data is used "blindly", i.e., as a sequence of bytes.
	This is straightforward for 8-bit images, but the caller must ensure that
	16-bit images originate from a short or unsigned short buffer with MSB first.
	For RGB images, the color bytes are interlaced in the order R - G - B - R - G - ...

	In addition to the buffer, the image dimensions width x height need to be provided
	(in pixels, not in bytes, meaning, the total number of bytes is bps*width*height)
	The parameter bps specifies the image type (1, 2, or 3 for 8-bit, 16-bit, and RGB,
	respecvtively).
	The comment string is optional. A NULL pointer may be passed.
*/


int tiffwrite (const char* fname, char* img, int width, int height, int bps, char* comment)
{
TIFF *tif;
long numbytes;
float tiff_dpi = 600.0;


	tif = TIFFOpen(fname, "w");
	if (!tif) return -1;

	/* Let's start with some general tags */

	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
	//TIFFSetField(tif, TIFFTAG_IMAGEDEPTH, depth);			/* Optional: Can save multi-slice images */

	/* Compression. By default, use LZW */

	TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

	if (comment && strlen(comment) > (size_t) 0) 
		TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, comment);

	/* Some necessary tags for which we specify dummy values, but such that
		these at least make some sort of sense */

	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, (int)2);
	TIFFSetField(tif, TIFFTAG_XRESOLUTION, tiff_dpi);
	TIFFSetField(tif, TIFFTAG_YRESOLUTION, tiff_dpi);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	
	/* now write format-specific tags and the image data */

	if (bps==1)					/* 1-byte grayscale */
	{
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,  8);
    	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,    PHOTOMETRIC_MINISBLACK);
	}
	else if (bps==2)			/* Signed short */
	{
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,  16);
    	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,    PHOTOMETRIC_MINISBLACK);
		TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT,   SAMPLEFORMAT_INT); /* Signed! */
	}
	else if (bps==3)			/* RGB, 8-bit channels */
	{
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
		TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,  8);
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,    PHOTOMETRIC_RGB);
	}
	else						/* Please make sure this does not happen :-(     */
	{
		TIFFClose (tif);
		return -1;
	}

	/* Actually write image data, this is the last step */

	numbytes = (long)bps * (long)width * (long)height;
	TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, height);
   	TIFFWriteEncodedStrip (tif, 0, img, numbytes);

	TIFFClose(tif);

	return 0;
}




	




/************************************************************************************

	Portable graphics formats, PGM and the likes.
	Note that netpbm files are single-slice only and we therefore
	save only the slice at st. The caller should make sure that the netpbm
	option is disabled for stacks.


************************************************************************************/



int pnm_write_8 (char* fname, unsigned char* img, int width, int height)
{
int errcode;
long numelems,l;
FILE* FP;
char hdr[256];

	sprintf (hdr, "P5 %d %d %d\n", width, height, 255);

	errcode=0;
	l = (long)width * (long)height;
	
	FP = fopen (fname, "wb");
	if (!FP) return -1;
	
	numelems = fwrite (hdr, sizeof (char), strlen(hdr), FP);
	
	numelems = fwrite (img, sizeof (char), l, FP);
	if (numelems!=l) 
	{
		fprintf (stderr, "PNM write warning: Fewer elements written than file size\n");
		errcode=-1;
	}
	fclose (FP);

	return errcode;
}


/* Write 16-bit PNM. For this, we actually scan the image data for the max value,
	and we enforce the 16-bit by making the max value larger than 255 if it is not
	-- otherwise, the PGM import filters would automatically interpret the data
	as 8-bit, which leads to incorrect read results.
*/

int pnm_write_16 (char* fname, short* img, int width, int height)
{
int errcode;
short maxval;
long numelems,l,i;
FILE* FP;
char hdr[256];

	errcode=0;
	l = (long)width * (long)height;
	maxval = 1023;			/* Guarantee 16-bit interpretation and pretend a minimum of 10-bit data */

	for (i=0; i<l; i++)
		if (maxval < img[i]) maxval = img[i];

	sprintf (hdr, "P5 %d %d %d\n", width, height, maxval);

	FP = fopen (fname, "wb");
	if (!FP) return -1;

	numelems = fwrite (hdr, sizeof (char), strlen(hdr), FP);

	numelems = fwrite (img, sizeof (char), l, FP);
	if (numelems!=l) 
	{
		fprintf (stderr, "PNM write warning: Fewer elements written than file size\n");
		errcode=-1;
	}
	fclose (FP);

	return errcode;
}


/* To have the same capabilities as tiff write, also provide PNM RGB. Logically,
	this is no longer a pGm, and it has therefore a different header.
*/

int pnm_write_rgb (char* fname, unsigned char* img, int width, int height)
{
int errcode;
long numelems,l;
FILE* FP;
char hdr[256];

	sprintf (hdr, "P6 %d %d %d\n", width, height, 255);

	errcode=0;
	l = (long)width * (long)height * 3;			/* 3 bytes per pixel */
	
	FP = fopen (fname, "wb");
	if (!FP) return -1;
	
	numelems = fwrite (hdr, sizeof (char), strlen(hdr), FP);
	
	numelems = fwrite (img, sizeof (char), l, FP);
	if (numelems!=l) 
	{
		fprintf (stderr, "PNM write warning: Fewer elements written than file size\n");
		errcode=-1;
	}
	fclose (FP);

	return errcode;
}





/*******************************************************************************/



