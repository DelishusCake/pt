#ifndef RENDER_H
#define RENDER_H

#include "core.h"
#include "geom.h"

typedef struct
{
	f32 t;
	v3 normal;
	v3 position;
} hit_t;

typedef struct
{
	v3  center;
	f32 radius;
} sphere_t;

#define MAX_SPHERES	256

typedef struct
{
	u32 sphere_count;
	sphere_t spheres[MAX_SPHERES];
} world_t;

bool world_hit(const world_t *world, ray_t ray,
	f32 min_t, f32 max_t, hit_t *hit);

typedef struct
{
	v3 position;
	v3 x, y, z;
	v3 h, v, f;
} camera_t;

camera_t look_at(
	v3 position, v3 at, v3 up, 
	f32 fov, f32 aspect_ratio);
ray_t camera_ray(const camera_t *camera, f32 u, f32 v);

#endif