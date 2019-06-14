#ifndef RENDER_H
#define RENDER_H

#include "core.h"
#include "util.h"
#include "geom.h"

#include "image.h"
#include "world.h"

void render(const world_t *world, 
	const camera_t *camera, 
	i32 samples, i32 bounces,
	image_t *image, rect_t area);

#endif