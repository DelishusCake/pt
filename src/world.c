#include "world.h"

// Get the AABB for a sphere
aabb_t sphere_aabb(v3 center, f32 radius)
{
	aabb_t aabb;
	aabb.min = v3_sub(center, V3(radius, radius, radius));
	aabb.max = v3_add(center, V3(radius, radius, radius));
	return aabb;
};

// Hit test a sphere against a ray
static bool sphere_hit(const sphere_t *sphere, ray_t ray, 
	f32 t_min, f32 t_max, hit_t *hit)
{
	const v3 oc = v3_sub(ray.origin, sphere->center);
	const f32 a = v3_dot(ray.direction, ray.direction);
	const f32 b = v3_dot(ray.direction, oc);
	const f32 c = v3_dot(oc, oc) - f32_square(sphere->radius);
	
	const f32 det = b*b - a*c;
	if (det > 0.f)
	{
		const f32 t_0 = (-b - f32_sqrt(det)) / (a);
		const f32 t_1 = (-b + f32_sqrt(det)) / (a);
		const f32 t = min(t_0, t_1);

		if ((t > t_min) && (t < t_max))
		{
			const v3 position = ray_point(ray, t);
			const v3 normal = v3_norm(v3_sub(position, sphere->center));

			hit->t = t;
			hit->normal = normal;
			hit->position = position;
			hit->material = sphere->material;
			return true;
		}
	}
	return false;
};

static int bvh_compare_x(const void *a, const void *b)
{
	const sphere_t *sphere_a = *(sphere_t**) a;
	const sphere_t *sphere_b = *(sphere_t**) b;
	return ((sphere_a->aabb.min.x - sphere_b->aabb.min.x) < 0.f) ? -1 : 1;
};
static int bvh_compare_y(const void *a, const void *b)
{
	const sphere_t *sphere_a = *(sphere_t**) a;
	const sphere_t *sphere_b = *(sphere_t**) b;
	return ((sphere_a->aabb.min.y - sphere_b->aabb.min.y) < 0.f) ? -1 : 1;
};
static int bvh_compare_z(const void *a, const void *b)
{
	const sphere_t *sphere_a = *(sphere_t**) a;
	const sphere_t *sphere_b = *(sphere_t**) b;
	return ((sphere_a->aabb.min.z - sphere_b->aabb.min.z) < 0.f) ? -1 : 1;
};
// Build a BVH recursively, based on a list of spheres
static bvh_t* build_bvh(sphere_t **spheres, u32 sphere_count)
{
	if (sphere_count > 2)
	{
		// Sort spheres along a random axis
		const u32 axis = u32_rand(0, 2);
		switch (axis)
		{
			case 0: qsort(spheres, sphere_count, sizeof(sphere_t*), bvh_compare_x); break;
			case 1: qsort(spheres, sphere_count, sizeof(sphere_t*), bvh_compare_y); break;
			case 2: qsort(spheres, sphere_count, sizeof(sphere_t*), bvh_compare_z); break;
		}
	}

	// Allocate a new BVH node
	bvh_t *bvh = malloc(sizeof(bvh_t));
	assert(bvh != NULL);
	memset(bvh, 0, sizeof(bvh_t));
	// If theres only one sphere left, it's a leaf
	if (sphere_count == 1)
	{
		bvh->leaf = true;
		bvh->aabb = spheres[0]->aabb;
		bvh->sphere = spheres[0];
	// If theres exactly two spheres left, they become leaves
	} else if (sphere_count == 2) {
		bvh->leaf = false;
		bvh->l = build_bvh(spheres + 0, 1);
		bvh->r = build_bvh(spheres + 1, 1);
		bvh->aabb = aabb_combine(bvh->l->aabb, bvh->r->aabb);
	} else {
		// Otherwise divide the list in half, create BVH trees for both sides
		const u32 half = (sphere_count / 2);

		bvh->leaf = false;
		bvh->l = build_bvh(spheres + 0,    half);
		bvh->r = build_bvh(spheres + half, sphere_count - half);
		bvh->aabb = aabb_combine(bvh->l->aabb, bvh->r->aabb);
	}
	return bvh;
};
void world_build_bvh(world_t *world)
{
	// Allocate a new sphere list for the BVH building routine to modify
	sphere_t **spheres = malloc(world->sphere_count*sizeof(sphere_t*));
	assert(spheres != NULL);
	// Add all the spheres to it
	for (u32 i = 0; i < world->sphere_count; i++)
		spheres[i] = (world->spheres + i);
	// Build the world BVH
	world->bvh = build_bvh(spheres, world->sphere_count);
	// Free the temp sphere list
	free(spheres);
};

#if USE_BVH
#define MAX_QUERY_LIST_SIZE	256

typedef struct
{
	// Cold data
	u32 count;
	sphere_t *sphere[MAX_QUERY_LIST_SIZE];
	// Hot data
	f32 t_hit[MAX_QUERY_LIST_SIZE] align_16;
	f32 radius[MAX_QUERY_LIST_SIZE] align_16;
	f32 center_x[MAX_QUERY_LIST_SIZE] align_16;
	f32 center_y[MAX_QUERY_LIST_SIZE] align_16;
	f32 center_z[MAX_QUERY_LIST_SIZE] align_16;
} sphere_list_t;

static bool sphere_list_hit(sphere_list_t *list, ray_t ray,
	f32 t_min, f32 t_max, hit_t *hit)
{
	// Get the next multiple of 4 for the list count
	const u32 count = nearest4(list->count);
	{
		// Fill in the end of the lists with 0s
		const u32 delta = (count - list->count);
		memset(list->t_hit + list->count, 0x00, delta*sizeof(f32));
		memset(list->radius + list->count, 0x00, delta*sizeof(f32));
		memset(list->center_x + list->count, 0x00, delta*sizeof(f32));
		memset(list->center_y + list->count, 0x00, delta*sizeof(f32));
		memset(list->center_z + list->count, 0x00, delta*sizeof(f32));
	}
	// Pre-load the ray vectors
	// Ray origin vector
	const __m128 origin_x = _mm_set_ps1(ray.origin.x);
	const __m128 origin_y = _mm_set_ps1(ray.origin.y);
	const __m128 origin_z = _mm_set_ps1(ray.origin.z);
	// Ray direction vector
	const __m128 direction_x = _mm_set_ps1(ray.direction.x);
	const __m128 direction_y = _mm_set_ps1(ray.direction.y);
	const __m128 direction_z = _mm_set_ps1(ray.direction.z);
	// The a term of the quadratic equation is constant for all spheres
	// a = direction * direction
	const __m128 a = _mm_dot_ps(
		direction_x, direction_y, direction_z,
		direction_x, direction_y, direction_z);
	// For each 4 spheres in the list
	for (u32 i = 0; i < count; i += 4)
	{
		// Load the radius value
		const __m128 radius = _mm_load_ps(list->radius + i);
		// Load the center vector
		const __m128 center_x = _mm_load_ps(list->center_x + i);
		const __m128 center_y = _mm_load_ps(list->center_y + i);
		const __m128 center_z = _mm_load_ps(list->center_z + i);
		// Calculate the oc term
		// oc = origin - center
		const __m128 oc_x = _mm_sub_ps(origin_x, center_x);
		const __m128 oc_y = _mm_sub_ps(origin_y, center_y);
		const __m128 oc_z = _mm_sub_ps(origin_z, center_z);
		// Calculate the b term of the quadtratic equation
		// b = direction * oc
		const __m128 b = _mm_dot_ps(
			direction_x, direction_y, direction_z,
			oc_x, oc_y, oc_z);
		// Calculate the c term of the quadtratic equation
		// c = oc*oc - radius^2
		const __m128 oc_dot_oc = _mm_dot_ps(
				oc_x, oc_y, oc_z,
				oc_x, oc_y, oc_z);
		const __m128 c = _mm_sub_ps(oc_dot_oc, _mm_mul_ps(radius, radius));

		// Calculate the determinant value
		// det = b*b - a*c
		const __m128 det = _mm_sub_ps(_mm_mul_ps(b,b), _mm_mul_ps(a,c));
		// Get the t value, calculate both possible t values and select the minimum
		// t = (-b ± sqrt(det)) / a
		const __m128 t_0 = _mm_div_ps(_mm_add_ps(_mm_neg_ps(b), _mm_sqrt_ps(det)), a);
		const __m128 t_1 = _mm_div_ps(_mm_sub_ps(_mm_neg_ps(b), _mm_sqrt_ps(det)), a);
		const __m128 t = _mm_min_ps(t_0, t_1);
		// Store the t value
		_mm_store_ps(list->t_hit + i, t);
	};

	// Get the smallest t value in the list
	i32 smallest_idx = -1;
	f32 smallest_t = hit->t;
	for (u32 i = 0; i < count; i++)
	{
		const f32 t = list->t_hit[i];
		if ((t > t_min) &&
			(t < t_max) &&
			(t < smallest_t))
		{
			smallest_t = t;
			smallest_idx = i;
		}
	};
	// If we found a t value
	if (smallest_idx != -1)
	{
		// Calculate the hit data as normal
		const f32 t = smallest_t;
		const sphere_t *sphere = list->sphere[smallest_idx];

		const v3 position = ray_point(ray, t);
		const v3 normal = v3_norm(v3_sub(position, sphere->center));

		hit->t = t;
		hit->normal = normal;
		hit->position = position;
		hit->material = sphere->material;
		return true;
	}
	return false;
};
static void bvh_query(const bvh_t *bvh, ray_t ray, 
	f32 t_min, f32 t_max, sphere_list_t *list)
{
	// If the ray intersects with this BVH node
	if (aabb_hit(bvh->aabb, ray, t_min, t_max))
	{
		// If this is a leaf
		if (bvh->leaf)
		{
			// Get the Sphere pointer
			sphere_t *sphere = bvh->sphere;
			// Push the sphere data to the list
			const u32 index = list->count++;

			list->t_hit[index] = INFINITY;
			list->sphere[index] = sphere;
			list->radius[index] = sphere->radius;
			list->center_x[index] = sphere->center.x;
			list->center_y[index] = sphere->center.y;
			list->center_z[index] = sphere->center.z;
		} else {
			// Test both children
			bvh_query(bvh->l, ray, t_min, t_max, list);
			bvh_query(bvh->r, ray, t_min, t_max, list);
		}
	};
};
#endif

bool world_hit(lin_alloc_t *temp_alloc, 
	const world_t *world, ray_t ray,
	f32 t_min, f32 t_max, hit_t *hit)
{
	bool result = false;

	// By default, non-hits have an infinite distance
	hit->t = INFINITY;
	#if USE_BVH
		// Allocate a 16-byte aligned query list
		const size_t align = 16;
		const size_t size = sizeof(sphere_list_t);
		sphere_list_t *list = lin_alloc_push(temp_alloc, size, align);
		assert(list != NULL);
		{
			// Reset the list count
			list->count = 0;
			// Query the BVH and build a list of spheres to test
			bvh_query(world->bvh, ray, t_min, t_max, list);
			// Hit test the sphere list
			result = sphere_list_hit(list, ray, t_min, t_max, hit);
		}
		lin_alloc_reset(temp_alloc);
	#else
		// Test against every sphere in the list
		for (u32 i = 0; i < world->sphere_count; i++)
		{
			hit_t tmp_hit;
			if (sphere_hit(world->spheres + i, ray, t_min, t_max, &tmp_hit))
			{
				result = true;
				if (tmp_hit.t < hit->t)
					*hit = tmp_hit;
			}
		};
	#endif
	return result;
};

camera_t look_at(
	v3 position, v3 at, v3 up, 
	f32 fov, f32 aperture, f32 aspect_ratio)
{
	const v3 z = v3_norm(v3_sub(position, at));
	const v3 x = v3_norm(v3_cross(up, z));
	const v3 y = v3_norm(v3_cross(z, x));

	const f32 focus = v3_len(v3_sub(position, at));

	const f32 theta = to_radians(fov);
	const f32 hh = f32_tan(theta/2);
	const f32 hw = hh * aspect_ratio;

	const v3 f = v3_scale(v3_sub(v3_sub(v3_scale(x, -hw), v3_scale(y, hh)), z), focus);
	const v3 h = v3_scale(x, hw*focus*2.f);
	const v3 v = v3_scale(y, hh*focus*2.f);

	camera_t camera;
	camera.x = x;
	camera.y = y;
	camera.z = z;

	camera.f = f;
	camera.h = h;
	camera.v = v;

	camera.aperture = aperture;
	camera.position = position;
	return camera;
};
ray_t camera_ray(const camera_t *camera, f32 u, f32 v)
{
	const v2 r = v2_scale(v2_unit_rand(), 0.5f*camera->aperture);
	const v3 offset = v3_add(v3_scale(camera->x, r.x), v3_scale(camera->y, r.y));

	ray_t ray;
	ray.origin = v3_add(camera->position, offset);
	ray.direction = v3_sub(v3_add(camera->f, v3_add(v3_scale(camera->h, u), v3_scale(camera->v, v))), offset);
	return ray;
}