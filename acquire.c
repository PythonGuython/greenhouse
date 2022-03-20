/*************************************************

	acquire.c

	Use Aravis API to acquire one frame from a
	(which one?) camera, save as PNG file

**************************************************/

/* Compile command:

gcc -pthread -I/home/pi/aravis/src -I/usr/include/glib-2.0/ -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include/ -I/usr/local/include/aravis-0.8 -L/home/pi/aravis/src/.libs/ -laravis-0.8 -lglib-2.0 -lgobject-2.0 -lpng -lpigpio -lrt -g acquire.c -o acquire

For the Aravis API documentation, see

https://aravisproject.github.io/docs/aravis-0.8/ArvCamera.html

*/


#include <png.h>
#include <assert.h>
#include <arv.h>
#include <stdlib.h>
#include <stdio.h>
#include <pigpiod_if2.h>

#include "tiffstuff.h"


int debuglevel;			/* Can be used to fprintf() debug messages */
double exposure;
char savefile[1024];
char sequence[255];
enum led_color{White, Blue, White_and_blue};

#define WHITE_LED_PIN 15
#define BLUE_LED_PIN 14




/*****************************************************

	An important help for debugging. dp (debug-print)
	is equivalent to fprintf (stderr, ...), but
	only prints if the priority level pri of the message
	exceeds the currently selected debug level.
	If debuglevel == 0, no messages are printed.
*/



void dp (int pri, char *format,...)
{
va_list args;
char buf[1024];

	if (pri > debuglevel) return;
    va_start(args, format);
    vsprintf(buf,format,args);
    fprintf (stderr,"%s", buf);
}



void show_error (GError **error)
{
	if ((error != NULL) && (*error != NULL))
		dp (0, "Error message: %s\n", (*error)->message);
	g_clear_error (error);
}



void arv_save_png (ArvBuffer *buffer, const char *filename)
{
size_t buffer_size;
char *buffer_data;
int i, width, height;
int bit_depth, arv_row_stride, color_type;


	/* First of all, verify that we are really received an *image* to save as png */

	assert (arv_buffer_get_payload_type(buffer) == ARV_BUFFER_PAYLOAD_TYPE_IMAGE);

	/* Next, get the image metainformation and the pointer to th epixel buffer */

	buffer_data = (char*)arv_buffer_get_data (buffer, &buffer_size); 				// raw data
	arv_buffer_get_image_region(buffer, NULL, NULL, &width, &height); 				// get width/height
	bit_depth = ARV_PIXEL_FORMAT_BIT_PER_PIXEL(arv_buffer_get_image_pixel_format(buffer)); // bit(s) per pixel
	arv_row_stride = width * bit_depth/8; // bytes per row, for constructing row pointers
	color_type = PNG_COLOR_TYPE_GRAY;

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	FILE * f = fopen(filename, "wb");
	png_init_io (png_ptr, f);
	png_set_IHDR (png_ptr, info_ptr, width, height, bit_depth, color_type,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info (png_ptr, info_ptr);

	png_bytepp rows = (png_bytepp)(png_malloc(png_ptr, height*sizeof(png_bytep)));

	for (i = 0; i < height; ++i)
			rows[i] = (png_bytep)(buffer_data + (height - i)*arv_row_stride);
	
	png_write_image(png_ptr, rows);
	png_write_end(png_ptr, NULL); 
	png_free(png_ptr, rows);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(f);
}




void arv_save_tiff (ArvBuffer *buffer, const char *filename)
{
size_t buffer_size;
char *buffer_data;
int i, width, height;
int bit_depth, arv_row_stride, color_type, bps;


	/* First of all, verify that we are really received an *image* before attempting to save it */

	assert (arv_buffer_get_payload_type(buffer) == ARV_BUFFER_PAYLOAD_TYPE_IMAGE);

	/* Next, get the image metainformation and the pointer to the pixel buffer */
	/* CAUTION: What format does the camera use for anything > 8 bits per pixel?
		If it is 16 bits, make sure we have MSB first.
		If it is 12 bits, make sure it is expanded to 16 bits; packed 12 bits are not acceptable 
	*/

	buffer_data = (char*)arv_buffer_get_data (buffer, &buffer_size); 				// raw data
	arv_buffer_get_image_region(buffer, NULL, NULL, &width, &height); 				// get width/height
	bit_depth = ARV_PIXEL_FORMAT_BIT_PER_PIXEL(arv_buffer_get_image_pixel_format(buffer)); // bit(s) per pixel
	bps = bit_depth / 8;					/* Bytes per sample, bytes per pixel */
	arv_row_stride = width * bps; // bytes per row, for constructing row pointers

	/* bps should tell the whole story, save the data now */

	tiffwrite (filename, buffer_data, width, height, bps, NULL );

}




int acquire_frame()
{
ArvCamera *camera;
ArvBuffer *buffer;
GError *error = NULL;
double exposure_time, exp_time_dummy;
double gain = 12;
double gain_min, gain_max;
const gchar *cam_vendor, *cam_model;
ArvPixelFormat pixelformat;


	exposure_time = 15000; 					/* in microseconds */
	camera = arv_camera_new (NULL, &error);
	if (!camera)
	{
		dp (0, "Error opening the camera object\n");
		show_error (&error);
		return -1;
	}

	cam_vendor = arv_camera_get_vendor_name (camera, &error);
	cam_model = arv_camera_get_model_name (camera, &error);
	show_error (&error);
	dp (1, "Found: Vendor %s, model %s\n", cam_vendor, cam_model);

	arv_camera_set_exposure_time_auto (camera, ARV_AUTO_OFF, &error);
	arv_camera_set_exposure_time (camera, exposure_time, &error);
	arv_camera_get_gain_bounds(camera, &gain_min, &gain_max, &error);
	arv_camera_set_gain_auto (camera, ARV_AUTO_OFF, &error);
	arv_camera_set_gain (camera, gain, &error);
	exp_time_dummy = arv_camera_get_exposure_time (camera, &error);
	show_error (&error);

	arv_camera_set_pixel_format (camera, ARV_PIXEL_FORMAT_MONO_16, &error);
	pixelformat = arv_camera_get_pixel_format (camera, &error);
	show_error (&error);
	if (pixelformat != ARV_PIXEL_FORMAT_MONO_16)
	{
		dp (1, "Warning: Unable to set 12-bit mono pixel format. Have %08H instead\n", pixelformat);
	}


	buffer = arv_camera_acquisition (camera, 0, &error);
	show_error (&error);
	if (ARV_IS_BUFFER (buffer))
	{
		dp (1, "Image successfully acquired. Now saving as a TIFF file.\n");
		arv_save_tiff (buffer, savefile);
//		printf ("Image successfully acquired. Now saving as a PNG file.\n");
//		arv_save_png (buffer, "test.png");
	}
	else
	{
		dp (0, "Failed to acquire a single image\n");
	}
	
	g_object_unref (camera);
	g_object_unref (buffer);

	return EXIT_SUCCESS;
}

void do_sequence(char *sequence)
{

    const char delims[3] = "-,";
    char *saveptr = sequence;
    char *token, *temp;
    char scount[2]; //could cause memory leaks? research malloc()?
    int dutycycle, count, err;
    enum led_color Led_color;
    
    count = 0;
    int pi;
    pi = pigpio_start(NULL, NULL);
    if (pi < 0)
    {
        fprintf (stderr, "Connection to pigpio daemon failed");
        return -1;
    }
    
    err = set_mode(pi, WHITE_LED_PIN, PI_OUTPUT);
    err = set_mode(pi, BLUE_LED_PIN, PI_OUTPUT);
    
    err = set_PWM_frequency(pi, WHITE_LED_PIN, 8000);
    err = set_PWM_frequency(pi, BLUE_LED_PIN, 8000);

    
    while ((token = strtok_r(saveptr, delims, &saveptr)))
    {
        //parse command line input into substrings
        printf("letter: %s\n", token);
        
        if (!strcmp(token, "b"))
        {
            Led_color = Blue;
        }
        else if (!strcmp(token, "w"))
        {
            Led_color = White;
        }
        else if (!strcmp(token, "bw"))
        {
            Led_color = White_and_blue;
        }
        else
        {
            dp (1, "Could not parse sequence");
            return;
        }
        
        token = strtok_r(saveptr, delims, &saveptr);
        
        dutycycle = atoi(token); //must be value from 0 to 255
        
        //create unique filename for each image
        char filename[64]; //May have to come back to this - could cause memory leaks I think?
        strcpy(filename, "sequence");
        sprintf(scount, "%d", count);
        strcat(filename, scount);
        strcpy(savefile, filename);
        
        if (Led_color == Blue)
        {
            err = set_PWM_dutycycle(pi, BLUE_LED_PIN, dutycycle);
            acquire_frame();
            err = set_PWM_dutycycle(pi, BLUE_LED_PIN, 0);
        }
        else if (Led_color == White)
        {
            err = set_PWM_dutycycle(pi, WHITE_LED_PIN, dutycycle);
            acquire_frame();
            err = set_PWM_dutycycle(pi, WHITE_LED_PIN, 0);
        }
        else if (Led_color == White_and_blue)
        {
            err = set_PWM_dutycycle(pi, BLUE_LED_PIN, dutycycle);
            err = set_PWM_dutycycle(pi, WHITE_LED_PIN, dutycycle);
            acquire_frame();
            err = set_PWM_dutycycle(pi, BLUE_LED_PIN, 0);
            err = set_PWM_dutycycle(pi, WHITE_LED_PIN, 0);
        }

        count++;
       }
}



/********************************************************************/


#define nextargi (--argc,atoi(*++argv))
#define nextargf (--argc,atof(*++argv))
#define nextargs (--argc,*++argv)


void prhelp()
{

	fprintf (stderr, "acquire: Get a frame from an Aravis-connected camera\n");
	fprintf (stderr, "valid options are:\n");
	fprintf (stderr, "-h --help         print this help text\n");
	fprintf (stderr, "-v --verbose      enable debug message output\n");
	fprintf (stderr, "-o                save output to file, -o 'name'\n");
	fprintf (stderr, "-e --exposure     set exposure time in microseconds, -e 1000.0\n");
	fprintf (stderr, "-s --sequence     acquire a sequence of images with PWM controlled LEDs at a specified duty cycle (see documentation)\n");
	fprintf (stderr, "-g --gain         set gain");
 
}



int main (int argc, char **argv)
{
int err;

	debuglevel = 0;
	exposure = 1000.0;
	strcpy (savefile, "test.tif");
	char *sequence;

    while (--argc && **++argv=='-') 
	{
		if (!strcmp(argv[0], "-o"))
			strcpy (savefile, nextargs);
		else if (!strcmp(argv[0],"-v") || !strcmp(argv[0],"--verbose"))
		{
			debuglevel++;
			dp (2, "Verbosity level raised to %d\n", debuglevel);
		}
		else if (!strcmp(argv[0], "-e") || !strcmp(argv[0],"--exposure"))
			exposure = nextargf;
		else if (!strcmp(argv[0],"-h") || !strcmp(argv[0],"--help"))
		{
			prhelp();
			return 0;
		}
		else if (!strcmp(argv[0],"-s") || !strcmp(argv[0],"--sequence"))
		{
            sequence = nextargs;
            do_sequence(sequence);
            return 0;
		}
	}

	acquire_frame();

}
