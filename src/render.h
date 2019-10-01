#ifndef RENDER_H
#define RENDER_H

#include "core.h"
#include "util.h"
#include "geom.h"

#include "image.h"
#include "world.h"

void render(
	// Temporary allocation space
	lin_alloc_t *temp_alloc,
	// Input world structure
	const world_t *world, 
	// Input camera structure
	const camera_t *camera, 
	// Rendering parameters
	i32 samples, i32 bounces,
	// Output image and area to render
	image_t *image, rect_t area);

#endif