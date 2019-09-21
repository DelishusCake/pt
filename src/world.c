#include "world.h"

aabb_t sphere_aabb(v3 center, f32 radius)
{
	aabb_t aabb;
	aabb.min = v3_sub(center, V3(radius, radius, radius));
	aabb.max = v3_add(center, V3(radius, radius, radius));
	return aabb;
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
static bvh_t* build_bvh(sphere_t **spheres, u32 sphere_count)
{
	if (sphere_count > 2)
	{
		const u32 axis = u32_rand(0, 2);
		switch (axis)
		{
			case 0: qsort(spheres, sphere_count, sizeof(sphere_t*), bvh_compare_x); break;
			case 1: qsort(spheres, sphere_count, sizeof(sphere_t*), bvh_compare_y); break;
			case 2: qsort(spheres, sphere_count, sizeof(sphere_t*), bvh_compare_z); break;
		}
	}

	bvh_t *bvh = malloc(sizeof(bvh_t));
	assert(bvh != NULL);
	memset(bvh, 0, sizeof(bvh_t));
	if (sphere_count == 1)
	{
		bvh->leaf = true;
		bvh->aabb = spheres[0]->aabb;
		bvh->sphere = spheres[0];
	} else if (sphere_count == 2) {
		bvh->leaf = false;
		bvh->l = build_bvh(spheres + 0, 1);
		bvh->r = build_bvh(spheres + 1, 1);
		bvh->aabb = aabb_combine(bvh->l->aabb, bvh->r->aabb);
	} else {
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
	sphere_t **spheres = malloc(world->sphere_count*sizeof(sphere_t*));
	assert(spheres != NULL);
	for (u32 i = 0; i < world->sphere_count; i++)
		spheres[i] = (world->spheres + i);

	world->bvh = build_bvh(spheres, world->sphere_count);
	
	free(spheres);
};

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
static bool bvh_hit(const bvh_t *bvh, ray_t ray, 
	f32 t_min, f32 t_max, hit_t *hit)
{
	if (aabb_hit(bvh->aabb, ray, t_min, t_max))
	{
		if (bvh->leaf)
		{
			hit_t data;
			if (sphere_hit(bvh->sphere, ray, t_min, t_max, &data))
			{
				*hit = data;
				return true;
			};
		} else {
			hit_t l_data, r_data;
			const bool hit_l = bvh_hit(bvh->l, ray, t_min, t_max, &l_data);
			const bool hit_r = bvh_hit(bvh->r, ray, t_min, t_max, &r_data);
			if (hit_l && hit_r)
			{
				*hit = (l_data.t < r_data.t) ? l_data : r_data;
				return true;
			};
			if (hit_l)
			{
				*hit = l_data;
				return true;
			}
			if (hit_r)
			{
				*hit = r_data;
				return true;
			}
		}
	};
	return false;
};
bool world_hit(lin_alloc_t *temp_alloc, 
	const world_t *world, ray_t ray,
	f32 t_min, f32 t_max, hit_t *hit)
{
	hit->t = INFINITY;
	
	#if USE_BVH
		return bvh_hit(world->bvh, ray, t_min, t_max, hit);
	#else
		bool result = false;
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
		return result;
	#endif
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