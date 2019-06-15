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

#if 0
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
bool world_hit(const world_t *world, ray_t ray,
	f32 t_min, f32 t_max, hit_t *hit)
{
	return bvh_hit(world->bvh, ray, t_min, t_max, hit);
};
#else
typedef struct
{
	u32 size, used;

	f32 *radius;
	f32 *center_x;
	f32 *center_y;
	f32 *center_z;

	material_t *material;
} sphere_list_t;

static inline __m128 _mm_dot_ps(
	__m128 a_x, __m128 a_y, __m128 a_z,
	__m128 b_x, __m128 b_y, __m128 b_z)
{
	const __m128 x = _mm_mul_ps(a_x, b_x);
	const __m128 y = _mm_mul_ps(a_y, b_y);
	const __m128 z = _mm_mul_ps(a_z, b_z);
	return _mm_add_ps(_mm_add_ps(x, y), z);
};
static inline __m128 _mm_square_ps(__m128 v)
{
	return _mm_mul_ps(v,v);
};
static inline __m128 _mm_neg_ps(__m128 v)
{
	return _mm_sub_ps(_mm_set1_ps(0.f), v); 
};
static bool sphere_list_hit(lin_alloc_t *temp_alloc, const sphere_list_t *list, ray_t ray, f32 t_min, f32 t_max, hit_t *hit)
{
	const u32 count = (list->used + 3) & ~0x03;
	
	const __m128 zero = _mm_set1_ps(0.f);
	const __m128 eps = _mm_set1_ps(1e-5);

	const __m128 origin_x = _mm_load1_ps(&ray.origin.x);
	const __m128 origin_y = _mm_load1_ps(&ray.origin.y);
	const __m128 origin_z = _mm_load1_ps(&ray.origin.z);

	const __m128 direction_x = _mm_load1_ps(&ray.direction.x);
	const __m128 direction_y = _mm_load1_ps(&ray.direction.y);
	const __m128 direction_z = _mm_load1_ps(&ray.direction.z);

	f32 *values = lin_alloc_push(temp_alloc, count*sizeof(f32), 16);
	assert(values != NULL);

	for (u32 i = 0; i < count; i += 4)
	{
		const __m128 radius = _mm_load_ps(list->radius + i);	
		const __m128 center_x = _mm_load_ps(list->center_x + i);
		const __m128 center_y = _mm_load_ps(list->center_y + i);
		const __m128 center_z = _mm_load_ps(list->center_z + i);
		
		const __m128 oc_x = _mm_sub_ps(origin_x, center_x);
		const __m128 oc_y = _mm_sub_ps(origin_y, center_y);
		const __m128 oc_z = _mm_sub_ps(origin_z, center_z);

		const __m128 a = _mm_dot_ps(
			direction_x, direction_y, direction_z, 
			direction_x, direction_y, direction_z);
		const __m128 b = _mm_dot_ps(
			direction_x, direction_y, direction_z,
			oc_x, oc_y, oc_z);
		const __m128 c = _mm_sub_ps(_mm_dot_ps(oc_x,oc_y,oc_z, oc_x,oc_y,oc_z), _mm_square_ps(radius));
		
		const __m128 det = _mm_sub_ps(_mm_square_ps(b), _mm_mul_ps(a, c));
		
		const __m128 t_0 = _mm_div_ps(_mm_add_ps(_mm_neg_ps(b), _mm_sqrt_ps(det)), a);
		const __m128 t_1 = _mm_div_ps(_mm_sub_ps(_mm_neg_ps(b), _mm_sqrt_ps(det)), a);
		const __m128 min_t = _mm_min_ps(t_0, t_1);
		const __m128 cmp = _mm_or_ps(_mm_cmpgt_ps(det, eps), _mm_cmplt_ps(det, _mm_neg_ps(eps)));
		
		const __m128 t = _mm_or_ps(_mm_and_ps(cmp, min_t), _mm_andnot_ps(cmp, zero));
		_mm_store_ps(values + i, t);
	}
	i32 smallest_index = -1;
	f32 smallest_t = INFINITY;
	for (u32 i = 0; i < list->used; i++)
	{
		const f32 t = values[i];
		if ((t != 0.f) && (t > t_min) && (t < t_max) && (t < smallest_t))
		{
			smallest_t = t;
			smallest_index = i;
		};
	};
	if (smallest_index != -1)
	{
		const v3 position = ray_point(ray, smallest_t);
		const v3 center = V3(
			list->center_x[smallest_index],
			list->center_y[smallest_index],
			list->center_z[smallest_index]);
		const v3 normal = v3_norm(v3_sub(position, center));

		hit->t = smallest_t;
		hit->normal = normal;
		hit->position = position;
		hit->material = list->material[smallest_index];
		return true;
	}
	return false;
};
static void build_sphere_list(sphere_list_t *list, const bvh_t *bvh, ray_t ray, f32 t_min, f32 t_max)
{
	if (aabb_hit(bvh->aabb, ray, t_min, t_max))
	{
		if (bvh->leaf)
		{
			assert((list->used+1) < list->size);
			const u32 index = list->used++;

			list->center_x[index] = bvh->sphere->center.x;
			list->center_y[index] = bvh->sphere->center.y;
			list->center_z[index] = bvh->sphere->center.z;
			list->radius[index] = bvh->sphere->radius;
			list->material[index] = bvh->sphere->material;
		} else {
			build_sphere_list(list, bvh->l, ray, t_min, t_max);
			build_sphere_list(list, bvh->r, ray, t_min, t_max);
		}
	};
};
bool world_hit(lin_alloc_t *temp_alloc,
	const world_t *world, ray_t ray, f32 t_min, f32 t_max, hit_t *hit)
{
	bool result = false;

	sphere_list_t list;
	list.used = 0;
	list.size = 128;
	list.radius   = lin_alloc_push(temp_alloc, list.size*sizeof(f32), 16);
	list.center_x = lin_alloc_push(temp_alloc, list.size*sizeof(f32), 16);
	list.center_y = lin_alloc_push(temp_alloc, list.size*sizeof(f32), 16);
	list.center_z = lin_alloc_push(temp_alloc, list.size*sizeof(f32), 16);
	list.material = lin_alloc_push(temp_alloc, list.size*sizeof(material_t), 0);

	build_sphere_list(&list, world->bvh, ray, t_min, t_max);
	
	if (list.used > 0)
	{
		result = sphere_list_hit(temp_alloc, &list, ray, t_min, t_max, hit);
	};
	lin_alloc_reset(temp_alloc);
	return result;
};
#endif

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