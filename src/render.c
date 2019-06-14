#include "render.h"

typedef struct
{
	f32 t;
	v3 normal;
	v3 position;
	material_t material;
} hit_t;

static bool sphere_hit(const sphere_t *sphere, ray_t ray, 
	f32 min_t, f32 max_t, hit_t *hit)
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

		if ((t < hit->t) && (t > min_t) && (t < max_t))
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
static bool world_hit(const world_t *world, ray_t ray,
	f32 min_t, f32 max_t, hit_t *hit)
{
	bool result = false;
	for (u32 i = 0; i < world->sphere_count; i++)
	{
		const sphere_t *sphere = world->spheres + i;
		if (sphere_hit(sphere, ray, min_t, max_t, hit))
		{
			result = true;
		}
	};
	return result;
};

camera_t look_at(
	v3 position, v3 at, v3 up, 
	f32 fov, f32 aspect_ratio)
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
	camera.position = position;
	camera.x = x;
	camera.y = y;
	camera.z = z;

	camera.f = f;
	camera.h = h;
	camera.v = v;
	return camera;
};
static ray_t camera_ray(const camera_t *camera, f32 u, f32 v)
{
	ray_t ray;
	ray.origin = camera->position;
	ray.direction = v3_add(camera->f, v3_add(v3_scale(camera->h, u), v3_scale(camera->v, v)));
	return ray;
}

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
		case MATERIAL_METAL:
			return scatter_metal(ray, hit, attenuation, new_ray);
		case MATERIAL_LAMBERTIAN:
			return scatter_lambertian(ray, hit, attenuation, new_ray);
		case MATERIAL_DIELECTRIC:
			return scatter_dielectric(ray, hit, attenuation, new_ray);
		default: break;
	}
	return false;
}

static v3 sample(const world_t *world, ray_t ray, i32 max_depth, i32 depth)
{
	const f32 min_t = 0.001f;
	const f32 max_t = FLT_MAX;

	hit_t hit;
	hit.t = INFINITY;
	if (world_hit(world, ray, min_t, max_t, &hit))
	{	
		ray_t new_ray;
		v3 attenuation;

		if ((depth < max_depth) && scatter(ray, &hit, &attenuation, &new_ray))
		{
			return v3_mul(attenuation, sample(world, new_ray, max_depth, (depth+1)));
		}
		return V3(0.f, 0.f, 0.f);
	}
	return world->background;
};

static inline u32 srgb(v3 color)
{
	const f32 exp = (1.f/2.2f);
	const u8 r = (u32)(f32_pow(f32_saturate(color.r), exp) * 255.f + 0.5f) & 0xFF;
	const u8 g = (u32)(f32_pow(f32_saturate(color.g), exp) * 255.f + 0.5f) & 0xFF;
	const u8 b = (u32)(f32_pow(f32_saturate(color.b), exp) * 255.f + 0.5f) & 0xFF;
	return (0xFF << 24) | (b << 16) | (g << 8) | (r << 0);
};
void render(
	const world_t *world, 
	const camera_t *camera, 
	i32 samples, i32 bounces,
	image_t *image, rect_t area)
{
	u8 *row = (image->pixels + 
		(area.x*image->bpp) + 
		(area.y*image->stride));
	for (u32 j = area.y; j < (area.y+area.h); j++)
	{
		u32 *pixel = (u32*) row;
		for (u32 i = area.x; i < (area.x+area.w); i++)
		{
			v3 color = V3(0.f, 0.f, 0.f);
			for (u32 s = 0; s < samples; s++)
			{
				const f32 u = (((f32) i + f32_rand()) / (f32) image->width);
				const f32 v = (((f32) j + f32_rand()) / (f32) image->height);

				ray_t ray = camera_ray(camera, u, v);

				color = v3_add(color, sample(world, ray, bounces, 0));
			}
			color = v3_scale(color, (1.f / (f32) samples));

			*pixel++ = srgb(color);
		}
		row += image->stride;
	};
};