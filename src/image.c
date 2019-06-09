#include "image.h"

void image_alloc(image_t *image, u32 width, u32 height)
{
	const size_t bpp  = 4;
	const size_t size = width*height*bpp;

	u8 *pixels = malloc(size);
	assert(pixels != NULL);

	image->width = width;
	image->height = height;
	image->stride = width*bpp;
	image->pixels = pixels;
};
void image_free(image_t *image)
{
	free(image->pixels);
};

void image_save(const image_t *image, const char *file_name)
{
	const size_t bpp = 4;
	stbi_write_png(file_name,
		image->width, image->height, bpp,
		image->pixels, image->stride);
};
