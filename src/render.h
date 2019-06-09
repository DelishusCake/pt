#ifndef RENDER_H
#define RENDER_H

#include "core.h"
#include "util.h"
#include "geom.h"

#include "image.h"

#define MAX_SPHERES	256

typedef struct
{
	// Geometry data
	v3  center;
	f32 radius;
	// Material data
	v3  albedo;
} sphere_t;
typedef struct
{
	u32 sphere_count;
	sphere_t spheres[MAX_SPHERES];
} world_t;

typedef struct
{
	v3 position;
	v3 x, y, z;
	v3 h, v, f;
} camera_t;

camera_t look_at(
	v3 position, v3 at, v3 up, 
	f32 fov, f32 aspect_ratio);

typedef struct
{
	i32 x, y, w, h;
} rect_t;

void render(const world_t *world, 
	const camera_t *camera, 
	i32 samples, i32 bounces,
	image_t *image, rect_t area);

#endif