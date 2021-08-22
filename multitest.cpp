#include "~/aravis/src/arv.h"
#include <stdlib.h>
#include <stdio.h>

int main()
{
    ArvCamera *camera;
	ArvBuffer *buffer;

    camera = arv_camera_new(NULL, NULL);

    if (ARV_IS_CAMERA (camera)) {
        printf("camera found\n")
    }

    return 0;
}