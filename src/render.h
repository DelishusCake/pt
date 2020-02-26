#ifndef RENDER_H
#define RENDER_H

#include "core.h"
#include "util.h"
#include "geom.h"

#include "framebuffer.h"

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
	// Output framebuffer and area to render
	framebuffer_t *framebuffer, rect_t area);

void draw_bvh(const camera_t *camera, const bvh_t *bvh, framebuffer_t *framebuffer);

#endif