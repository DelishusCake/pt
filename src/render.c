#include "render.h"

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

static v3 sample(const world_t *world, ray_t ray, i32 bounces)
{
	const f32 min_t = 0.01f;
	const f32 max_t = FLT_MAX;

	v3 acc = V3(1.f, 1.f, 1.f);
	v3 color = V3(0.f, 0.f, 0.f);

	for (u32 i = 0; i < bounces; i++)
	{
		hit_t hit;
		if (!world_hit(world, ray, min_t, max_t, &hit))
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

				ray_t ray = camera_ray(camera, u,v);

				color = v3_add(color, sample(world, ray, bounces));
			}
			color = v3_scale(color, (1.f / (f32) samples));

			*pixel++ = srgb(color);
		}
		row += image->stride;
	};
};