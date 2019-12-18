#include "render.h"

void framebuffer_alloc(framebuffer_t *framebuffer, i32 w, i32 h)
{
	framebuffer->width = w;
	framebuffer->height = h;
	framebuffer->pixels = malloc(w*h*sizeof(v3));
	assert(framebuffer->pixels != NULL);
	memset(framebuffer->pixels, 0, w*h*sizeof(v3));
};
void framebuffer_free(framebuffer_t *framebuffer)
{
	free(framebuffer->pixels);
};
static inline void framebuffer_set(framebuffer_t *framebuffer, i32 x, i32 y, v3 color)
{
	if ((x >= 0) && (x < framebuffer->width) &&
		(y >= 0) && (y < framebuffer->height))
	{
		framebuffer->pixels[y*framebuffer->width + x] = color;
	}
};

static inline f32 schlick(f32 cos, f32 ref_idx)
{
	const f32 r_0 = f32_square((1.f - ref_idx) / (1.f + ref_idx));
	return r_0 + (1.f - r_0)*f32_pow((1.f-cos), 5);
};
static inline bool refract(v3 v, v3 n, f32 ni_over_nt, v3 *refracted)
{
	const v3 uv = v3_norm(v);
	const f32 dt = v3_dot(uv, n);
	const f32 det = 1.f - f32_square(ni_over_nt)*(1.f - f32_square(dt));
	if (det > 0.f)
	{
		*refracted = v3_sub(
			v3_scale(v3_sub(uv, v3_scale(n, dt)), ni_over_nt),
			v3_scale(n, f32_sqrt(det))
		);
		return true;
	}
	return false;
};

static bool scatter_lambertian(ray_t ray, const hit_t *hit, 
	v3 *attenuation, ray_t *new_ray)
{
	v3 target = v3_add(v3_add(hit->position, hit->normal), v3_unit_rand());

	new_ray->origin = hit->position;
	new_ray->direction = v3_norm(v3_sub(target, hit->position));

	*attenuation = hit->material.albedo;
	return true;
};
static bool scatter_metal(ray_t ray, const hit_t *hit, 
	v3 *attenuation, ray_t *new_ray)
{
	v3 reflected = v3_refl(ray.direction, hit->normal);

	new_ray->origin = hit->position;
	new_ray->direction = v3_add(reflected, v3_scale(v3_unit_rand(), hit->material.fuzz));

	*attenuation = hit->material.albedo;
	return (v3_dot(reflected, hit->normal) > 0.f);
};
static bool scatter_dielectric(ray_t ray, const hit_t *hit, 
	v3 *attenuation, ray_t *new_ray)
{
	const f32 eps = 1e-5;

	*attenuation = hit->material.albedo;

	v3 out_normal;
	f32 cos, ni_over_nt;
	if (v3_dot(ray.direction, hit->normal) > eps)
	{
		out_normal = v3_neg(hit->normal);
		ni_over_nt = hit->material.refractivity;
		cos = hit->material.refractivity*v3_dot(ray.direction, hit->normal) / v3_len(ray.direction);
	} else {
		out_normal = hit->normal;
		ni_over_nt = 1.f / hit->material.refractivity;
		cos = -v3_dot(ray.direction, hit->normal) / v3_len(ray.direction);
	}
	
	v3 direction;
	v3 refr_direction;
	v3 refl_direction = v3_refl(ray.direction, hit->normal);
	if (refract(ray.direction, out_normal, ni_over_nt, &refr_direction))
	{
		const f32 refl_probability = schlick(cos, hit->material.refractivity);
		direction = (f32_rand() < refl_probability) ? refl_direction : refr_direction;;
	} else {
		direction = refl_direction;
	}
	
	new_ray->origin = hit->position;
	new_ray->direction = direction;
	return true;
};
static bool scatter(ray_t ray, const hit_t *hit, 
	v3 *attenuation, ray_t *new_ray)
{
	switch (hit->material.type)
	{
		case MATERIAL_METAL:		return scatter_metal(ray, hit, attenuation, new_ray);
		case MATERIAL_LAMBERTIAN:	return scatter_lambertian(ray, hit, attenuation, new_ray);
		case MATERIAL_DIELECTRIC:	return scatter_dielectric(ray, hit, attenuation, new_ray);
		default: break;
	}
	return false;
}

static v3 sample(lin_alloc_t *temp_alloc, const world_t *world, ray_t ray, i32 bounces)
{
	const f32 min_t = 0.001f;
	const f32 max_t = FLT_MAX;

	v3 acc = V3(1.f, 1.f, 1.f);
	v3 color = V3(0.f, 0.f, 0.f);

	for (u32 i = 0; i < bounces; i++)
	{
		hit_t hit;
		if (!world_hit(temp_alloc, world, ray, min_t, max_t, &hit))
		{
			color = v3_add(color, v3_mul(acc, world->background));
			break;
		}

		color = v3_add(color, v3_mul(acc, hit.material.emittance));

		ray_t new_ray;
		v3 attenuation;
		if (scatter(ray, &hit, &attenuation, &new_ray))
		{
			acc = v3_mul(acc, attenuation);
		}
		ray = new_ray;
	};
	return color;
};

void render(lin_alloc_t *temp_alloc,
	const world_t *world, 
	const camera_t *camera, 
	i32 samples, i32 bounces,
	framebuffer_t *framebuffer, rect_t area)
{
	// For each row of the area to render
	for (u32 j = area.y; j < (area.y+area.h); j++)
	{
		// Iterate over each pixel
		for (u32 i = area.x; i < (area.x+area.w); i++)
		{
			// Final output color in RGB space
			v3 color = V3(0.f, 0.f, 0.f);
			// For each sample
			for (u32 s = 0; s < samples; s++)
			{
				// Get the current UV of this sample
				const f32 u = (((f32) i + f32_rand()) / (f32) framebuffer->width);
				const f32 v = (((f32) j + f32_rand()) / (f32) framebuffer->height);
				// Generate a ray from the camera to the sample
				ray_t ray = camera_ray(camera, u,v);
				// Generate a sample and add it to the color
				color = v3_add(color, sample(temp_alloc, world, ray, bounces));
			}
			// Normalize the output color by the number of samples
			color = v3_scale(color, (1.f / (f32) samples));
			// Store the final color
			framebuffer->pixels[j*framebuffer->width + i] = color;
		}
	};
};

static void draw_line(framebuffer_t *framebuffer, v2 a, v2 b, v3 color)
{
	i32 x0 = a.x;
	i32 y0 = a.y;

	i32 x1 = b.x;
	i32 y1 = b.y;

	bool steep = false; 
	if (abs(x0-x1) < abs(y0-y1))
	{ 
		swap(i32, x0, y0); 
		swap(i32, x1, y1); 
		steep = true; 
	} 
	if (x0 > x1)
	{ 
		swap(i32, x0, x1); 
		swap(i32, y0, y1); 
	} 

	i32 dx = x1-x0; 
	i32 dy = y1-y0; 
	i32 derror2 = abs(dy)*2; 
	i32 error2 = 0; 
	i32 y = y0; 
	for (i32 x = x0; x <= x1; x++)
	{ 
		const i32 i = steep ? y : x;
		const i32 j = steep ? x : y;
		framebuffer_set(framebuffer, i, j, color);

		error2 += derror2; 
		if (error2 > dx)
		{ 
			y += (y1>y0) ? 1 : -1; 
			error2 -= dx*2;
		} 
	} 
};
static v4 persp(v4 v)
{
	return V4(v.x/v.w, v.y/v.w, v.z/v.w, v.w);
};
static v4 clip(m44 viewport, v4 vertex)
{
	return persp(m44_transform(viewport, vertex));	
};
static void draw_aabb(m44 camera, aabb_t aabb, framebuffer_t *framebuffer)
{
	const m44 viewport = m44_viewport(0,0,framebuffer->width,framebuffer->height);
	const v3 color = V3(1.f, 0.f, 0.f);
	{
		const v4 min = clip(viewport, m44_transform(camera, V4(aabb.min.x, aabb.min.y, aabb.min.z, 1.f)));
		const v4 max = clip(viewport, m44_transform(camera, V4(aabb.max.x, aabb.max.y, aabb.min.z, 1.f)));

		draw_line(framebuffer, V2(min.x, min.y), V2(max.x, min.y), color);
		draw_line(framebuffer, V2(max.x, min.y), V2(max.x, max.y), color);
		draw_line(framebuffer, V2(max.x, max.y), V2(min.x, max.y), color);
		draw_line(framebuffer, V2(min.x, max.y), V2(min.x, min.y), color);
	}
	#if 0
	{
		const v4 min = clip(viewport, m44_transform(camera, V4(aabb.min.x, aabb.min.y, aabb.max.z, 1.f)));
		const v4 max = clip(viewport, m44_transform(camera, V4(aabb.max.x, aabb.max.y, aabb.max.z, 1.f)));

		draw_line(framebuffer, V2(min.x, min.y), V2(max.x, min.y), color);
		draw_line(framebuffer, V2(max.x, min.y), V2(max.x, max.y), color);
		draw_line(framebuffer, V2(max.x, max.y), V2(min.x, max.y), color);
		draw_line(framebuffer, V2(min.x, max.y), V2(min.x, min.y), color);
	}
	#endif
};
static void draw_bvh_node(m44 camera, const bvh_t *bvh, framebuffer_t *framebuffer)
{
	if (!bvh->leaf)
	{
		draw_bvh_node(camera, bvh->l, framebuffer);
		draw_bvh_node(camera, bvh->r, framebuffer);
	} else {
		draw_aabb(camera, bvh->aabb, framebuffer);
	}
};
void draw_bvh(const camera_t *camera, const bvh_t *bvh, framebuffer_t *framebuffer)
{
	const f32 aspect = ((f32) framebuffer->width / (f32) framebuffer->height);
	const m44 p = m44_perspective(
		to_radians(camera->fov), 
		aspect, 
		0.1f, 10.f);
	const m44 v = m44_lookAt(camera->position, camera->at, camera->up);
	draw_bvh_node(m44_mul(p, v), bvh, framebuffer);
};
