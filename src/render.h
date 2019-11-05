#ifndef RENDER_H
#define RENDER_H

#include "core.h"
#include "util.h"
#include "geom.h"

#include "world.h"

typedef struct
{
	i32 width;
	i32 height;
	v3 *pixels;
} framebuffer_t;

void framebuffer_alloc(framebuffer_t *framebuffer, i32 w, i32 h);
void framebuffer_free(framebuffer_t *framebuffer);

void render(
	// Temporary allocation space
	lin_alloc_t *temp_alloc,
	// Input world structure
	const world_t *world, 
	// Input camera structure
	const camera_t *camera, 
	// Rendering parameters
	i32 samples, i32 bounces,
	// Output framebuffer and area to render
	framebuffer_t *framebuffer, rect_t area);

#endif