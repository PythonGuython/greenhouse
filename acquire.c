    #include <png.h>
    #include <assert.h>
    #include <arv.h>
    #include <stdlib.h>
    #include <stdio.h>

    double exposure_time = 1000; //in us

    void arv_save_png(ArvBuffer * buffer, const char * filename);

    int main (int argc, char **argv)
    {
            ArvCamera *camera;
            ArvBuffer *buffer;
            
            GError *error = NULL;

            camera = arv_camera_new (NULL, &error);

            arv_camera_set_exposure_time(camera, exposure_time, &error);
            buffer = arv_camera_acquisition (camera, 0, &error);
            if (ARV_IS_BUFFER (buffer)){
                    printf ("Image successfully acquired\n");
                    FILE * fp;
                    fp=fopen("/home/pi/test.png", "r");
                    arv_save_png(buffer, "test.png");
            }else{
                    printf ("Failed to acquire a single image\n");
            }
            g_clear_object (&camera);
            g_clear_object (&buffer);

            return EXIT_SUCCESS;
    }



    void arv_save_png(ArvBuffer * buffer, const char * filename)
    {
            assert(arv_buffer_get_payload_type(buffer) == ARV_BUFFER_PAYLOAD_TYPE_IMAGE);

            size_t buffer_size;
            char * buffer_data = (char*)arv_buffer_get_data(buffer, &buffer_size); // raw data
            int width;
            int height;
            arv_buffer_get_image_region(buffer, NULL, NULL, &width, &height); // get width/height
            int bit_depth = ARV_PIXEL_FORMAT_BIT_PER_PIXEL(arv_buffer_get_image_pixel_format(buffer)); // bit(s) per pixel

            int arv_row_stride = width * bit_depth/8; // bytes per row, for constructing row pointers
            int color_type = PNG_COLOR_TYPE_GRAY;

            png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
            png_infop info_ptr = png_create_info_struct(png_ptr);
            FILE * f = fopen(filename, "wb");
            png_init_io(png_ptr, f);
            png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
                    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
            png_write_info(png_ptr, info_ptr);

            png_bytepp rows = (png_bytepp)(png_malloc(png_ptr, height*sizeof(png_bytep)));
            int i =0;
            for (i = 0; i < height; ++i)
                    rows[i] = (png_bytep)(buffer_data + (height - i)*arv_row_stride);
            
            png_write_image(png_ptr, rows);
            png_write_end(png_ptr, NULL); 
            png_free(png_ptr, rows);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            fclose(f);
    }