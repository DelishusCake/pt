#ifndef IMAGE_H
#define IMAGE_H

#include "core.h"

#include <stb_image_write.h>

typedef struct 
{
	u32 width;
	u32 height;
	u32 stride;
	u8 *pixels;
} image_t;

void image_alloc(image_t *image, u32 width, u32 height);
void image_free(image_t *image);

void image_save(const image_t *image, const char *file_name);

#endif