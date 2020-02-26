#include "framebuffer.h"

void framebuffer_alloc(framebuffer_t *framebuffer, i32 w, i32 h)
{
	framebuffer->width = w;
	framebuffer->height = h;
	framebuffer->pixels = malloc(w*h*sizeof(v3));
	assert(framebuffer->pixels != NULL);
	memset(framebuffer->pixels, 0, w*h*sizeof(v3));
};
void framebuffer_free(framebuffer_t *framebuffer)
{
	free(framebuffer->pixels);
};

static inline u32 srgb(v3 color)
{
	const f32 exp = (1.f/2.2f);
	const u8 a = 0xFF;
	const u8 r = (u32)(f32_pow(f32_saturate(color.r), exp) * 255.f + 0.5f) & 0xFF;
	const u8 g = (u32)(f32_pow(f32_saturate(color.g), exp) * 255.f + 0.5f) & 0xFF;
	const u8 b = (u32)(f32_pow(f32_saturate(color.b), exp) * 255.f + 0.5f) & 0xFF;
	return (a << 24) | (b << 16) | (g << 8) | (r << 0);
};
void framebuffer_resolve(image_t *image, const framebuffer_t *framebuffer)
{
	// Get a pointer to the begining of the render area
	u8 *row = image->pixels;
	// For each row of the area to render
	for (u32 j = 0; j < image->height; j++)
	{
		// Iterate over each pixel
		u32 *pixel = (u32*) row;
		for (u32 i = 0; i < image->width; i++)
		{
			// Get the color
			const v3 color = framebuffer->pixels[j*framebuffer->width + i];
			// Store the pixel in srgb space
			*pixel++ = srgb(color);
		}
		// Iterate to the next row
		row += image->stride;
	};
};

#if 0
void framebuffer_filter(image_t *image, const framebuffer_t *framebuffer)
{
	const i32 kernelSize = 3;
	const f32 id = 5000.f;
	const f32 cd = 2.f;

	u8 *row = image->pixels;
	for (u32 y = 0; y < framebuffer->height; y++)
	{
		// Iterate over each pixel
		u32 *pixel = (u32*) row;
		for (u32 x = 0; x < framebuffer->width; x++)
		{
			// Get the color
			const v3 color = framebuffer->pixels[y*framebuffer->width + x];

			i32 k_min_x = x - kernelSize;
			i32 k_min_y = y - kernelSize;

			i32 k_max_x = x + kernelSize;
			i32 k_max_y = y + kernelSize;

			f32 s_weight = 0.f;
			v3 s_color = V3(0.f, 0.f, 0.f);
			for (u32 j = k_min_y; j <= k_max_y; j++)
			{
				for (u32 i = k_min_x; i <= k_max_x; i++)
				{
					const i32 n_x = clamp(i, 0, (framebuffer->width-1));
					const i32 n_y = clamp(j, 0, (framebuffer->height-1));

					const v3 n_color = framebuffer->pixels[n_y*framebuffer->width + n_x];

					f32 image_dist = f32_sqrt((f32) ((i-x)*(i-x)) + (f32) ((j-y)*(j-y)));
					f32 color_dist = f32_sqrt((f32) (
						((color.r - n_color.r)*(color.r - n_color.r)) +
						((color.g - n_color.g)*(color.g - n_color.g)) +
						((color.b - n_color.b)*(color.b - n_color.b))));
					f32 weight = 1.f / (f32_exp((image_dist/id)*(image_dist/id)*0.5f) * f32_exp((color_dist/cd)*(color_dist/cd)*0.5f));
					s_weight += weight;

					s_color.r += weight*color.r;
					s_color.g += weight*color.g;
					s_color.b += weight*color.b;
				}
			}
			s_color.r *= (1.f / s_weight);
			s_color.g *= (1.f / s_weight);
			s_color.b *= (1.f / s_weight);

			// Store the pixel in srgb space
			*pixel++ = srgb(s_color);
		}
		// Iterate to the next row
		row += image->stride;
	};
};
#endif
