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
#include <unistd.h>
#include <pigpiod_if2.h>

#define WHITE_LED_PIN 8
#define BLUE_LED_PIN 7





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



int acquire_frame()
{
ArvCamera *camera;
ArvBuffer *buffer;
GError *error = NULL;
double exposure_time, exp_time_dummy;
const gchar *cam_vendor, *cam_model;


	exposure_time = 1000.0; 					/* in microseconds */
	camera = arv_camera_new (NULL, &error);
	if (!camera)
	{
		fprintf (stderr, "Error opening the camera object\n");
		return -1;
	}

	cam_vendor = arv_camera_get_vendor_name (camera, &error);
	cam_model = arv_camera_get_model_name (camera, &error);
	fprintf (stderr, "Found: Vendor %s, model %s\n", cam_vendor, cam_model);

	arv_camera_set_exposure_time_auto (camera, ARV_AUTO_OFF, &error);
	arv_camera_set_exposure_time (camera, exposure_time, &error);
	exp_time_dummy = arv_camera_get_exposure_time (camera, &error);
	
	printf("exposure time is: %f us\n", exp_time_dummy);

	buffer = arv_camera_acquisition (camera, 0, &error);
	if (ARV_IS_BUFFER (buffer))
	{
		printf ("Image successfully acquired. Now saving as a PNG file.\n");
		arv_save_png (buffer, "test.png");
	}
	else
	{
		printf ("Failed to acquire a single image\n");
	}
	g_object_unref (camera);
	g_object_unref (buffer);

	return EXIT_SUCCESS;
}




int main (int argc, char **argv)
{
int pi;
int err;

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
    
    err = set_PWM_dutycycle(pi, WHITE_LED_PIN, 160);
    
    sleep(5);
    
    err = set_PWM_dutycycle(pi, WHITE_LED_PIN, 0);
    
    err = set_PWM_dutycycle(pi, BLUE_LED_PIN, 255);
    
    sleep(5);
    
    err = set_PWM_dutycycle(pi, BLUE_LED_PIN, 0);
    
    pigpio_stop(pi);
    
    


	acquire_frame();

}
