#pragma once

#include "targetver.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <png.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "zlib.lib")
#endif

typedef struct pcv_image {
    int width;
    int height;
    png_byte color_type;
    png_byte bit_depth;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *rows;
} pcv_image_t;

void read_png(char *file_name, struct pcv_image *image);
void write_png(struct pcv_image *image, char *file_name);
void process_image(struct pcv_image *image);
void blend_images(
    struct pcv_image *bottom,
	struct pcv_image *top,
	struct pcv_image *result
);
void release_image(struct pcv_image *image);
