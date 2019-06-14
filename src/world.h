#ifndef WORLD_H
#define WORLD_H

#include "core.h"
#include "util.h"
#include "geom.h"

#define MAX_SPHERES	256

typedef enum
{
	MATERIAL_NONE,
	MATERIAL_METAL,
	MATERIAL_DIELECTRIC,
	MATERIAL_LAMBERTIAN,
} material_type_t;
typedef struct
{
	material_type_t type;

	f32 fuzz;
	v3  albedo;
	v3  emittance;
	f32 refractivity;
} material_t;

typedef struct
{
	v3  center;
	f32 radius;
	aabb_t aabb;
	material_t material;
} sphere_t;

aabb_t sphere_aabb(v3 center, f32 radius);

decl_struct(bvh_t);

struct bvh_t
{
	bool leaf;
	aabb_t aabb;
	
	union
	{
		struct { bvh_t *l, *r; };
		sphere_t *sphere;
	};
};

typedef struct
{
	v3 background;

	bvh_t *bvh;

	u32 sphere_count;
	sphere_t spheres[MAX_SPHERES];
} world_t;

void world_build_bvh(world_t *world);

typedef struct
{
	f32 t;
	v3 normal;
	v3 position;
	material_t material;
} hit_t;

bool world_hit(const world_t *world, ray_t ray,
	f32 min_t, f32 max_t, hit_t *hit);

typedef struct
{
	v3 x, y, z;
	v3 h, v, f;
	v3 position;
	f32 aperture;
} camera_t;

camera_t look_at(
	v3 position, v3 at, v3 up, 
	f32 fov, f32 aperture, f32 aspect_ratio);
ray_t camera_ray(const camera_t *camera, f32 u, f32 v);

#endif