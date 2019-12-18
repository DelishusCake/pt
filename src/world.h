#ifndef WORLD_H
#define WORLD_H

#include "core.h"
#include "util.h"
#include "geom.h"

// Maximum number of spheres a world can contain
#define MAX_SPHERES	256

// Material data structure
typedef enum
{
	MATERIAL_NONE,
	MATERIAL_METAL,
	MATERIAL_DIELECTRIC,
	MATERIAL_LAMBERTIAN,
} material_type_t;
typedef struct
{
	// Material type marker
	material_type_t type;
	// The "fuzziness" of a metal material
	f32 fuzz;
	// Albedo color
	v3  albedo;
	// Emittance color
	v3  emittance;
	// Refractivity index
	f32 refractivity;
} material_t;

// Sphere data structure
typedef struct
{
	// Center position
	v3  center;
	// Radius
	f32 radius;
	// Bounding box
	aabb_t aabb;
	// Material data
	material_t material;
} sphere_t;

// Get the AABB for a sphere
aabb_t sphere_aabb(v3 center, f32 radius);

// BVH tree data structure
decl_struct(bvh_t);
struct bvh_t
{
	// Leaf marker, true for leaves, false for branches
	bool leaf;
	// Bounding box
	aabb_t aabb;
	
	union
	{
		// Branch data, left and right child pointers
		struct { bvh_t *l, *r; };
		// Leaf data, shape pointer
		sphere_t *sphere;
	};
};

// World data structure
typedef struct
{
	// World BVH containing all shapes
	bvh_t *bvh;
	// Background color, used when rays hit no shapes
	v3 background;
	// Sphere array
	u32 sphere_count;
	sphere_t spheres[MAX_SPHERES];
} world_t;

// Build the BVH for a world from it's sphere list
void world_build_bvh(world_t *world);

// Data structure for a hit record
typedef struct
{
	f32 t;
	v3 normal;
	v3 position;
	material_t material;
} hit_t;

// Raycast into the world, returns if a shape was hit
bool world_hit(
	// Temporary memory allocator, used for fast transient allocations
	lin_alloc_t *temp_alloc,
	// The world and ray input data
	const world_t *world, ray_t ray,
	// The ray length boundaries 
	f32 t_min, f32 t_max,
	// Output hit data structure
	hit_t *hit);

// Camera data structure
typedef struct
{
	// Camera space transformation matrix 
	v3 x, y, z;
	// Lens space transformation matrix
	v3 h, v, f;
	// Camera world space position 
	v3 position, at, up;
	// Camera aperture radius
	f32 aperture;
	// Camera FOV
	f32 fov;
} camera_t;

// Get the camera for a position
camera_t look_at(
	// Input positioning data
	v3 position, v3 at, v3 up,
	// Field of view, aperture, and aspect ratio 
	f32 fov, f32 aperture, f32 aspect_ratio);
// Get the outgoing ray from a camera, towards the lens space position
// NOTE: Will be randomly offset by a random amount based on the aperture for depth of field effects
ray_t camera_ray(const camera_t *camera, f32 u, f32 v);

#endif