#include "core.h"

#include <stb_image_write.h>

int main(int argc, const char *argv[])
{
	const u32 image_bpp    = 4;
	const u32 image_width  = 1920;
	const u32 image_height = 1080;
	const u32 image_stride = image_width*image_bpp;
	const u32 image_size = image_width*image_height*image_bpp;

	u8 *image = malloc(image_size);
	assert(image != NULL);
	memset(image, 0, image_size);
	
	u8 *row = image;
	for (u32 y = 0; y < image_height; y++)
	{
		u32 *pixel = (u32*) row;
		for (u32 x = 0; x < image_width; x++)
		{
			*pixel++ = (0xFF << 24) | (y << 16) | (x << 8);
		}
		row += image_stride;
	};
	stbi_write_png("image.png",
		image_width, image_height, image_bpp,
		image, image_stride);

	return 0;
}