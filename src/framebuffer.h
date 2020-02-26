#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "core.h"
#include "util.h"
#include "geom.h"

#include "image.h"

typedef struct
{
	i32 width;
	i32 height;
	v3 *pixels;
} framebuffer_t;

void framebuffer_alloc(framebuffer_t *framebuffer, i32 w, i32 h);
void framebuffer_free(framebuffer_t *framebuffer);

static inline void framebuffer_set(framebuffer_t *framebuffer, i32 x, i32 y, v3 color)
{
	if ((x >= 0) && (x < framebuffer->width) &&
		(y >= 0) && (y < framebuffer->height))
	{
		framebuffer->pixels[y*framebuffer->width + x] = color;
	}
};

void framebuffer_resolve(image_t *image, const framebuffer_t *framebuffer);

#endif