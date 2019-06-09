#include "render.h"

static bool sphere_hit(const sphere_t *sphere, ray_t ray, 
	f32 min_t, f32 max_t, hit_t *hit)
{
	const v3 oc = v3_sub(ray.origin, sphere->center);
	const f32 a = v3_dot(ray.direction, ray.direction);
	const f32 b = v3_dot(ray.direction, oc);
	const f32 c = v3_dot(oc, oc) - (sphere->radius*sphere->radius);
	
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
			return true;
		}
	}
	return false;
};
bool world_hit(const world_t *world, ray_t ray,
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
ray_t camera_ray(const camera_t *camera, f32 u, f32 v)
{
	ray_t ray;
	ray.origin = camera->position;
	ray.direction = v3_add(camera->f, v3_add(v3_scale(camera->h, u), v3_scale(camera->v, v)));
	return ray;
}