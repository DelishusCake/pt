#include "core.h"
#include "util.h"
#include "geom.h"

#include <stb_image_write.h>

typedef struct 
{
	u32 width;
	u32 height;
	u32 stride;
	u8 *pixels;
} image_t;

static void image_alloc(image_t *image, u32 width, u32 height)
{
	const size_t bpp  = 4;
	const size_t size = width*height*bpp;

	u8 *pixels = malloc(size);
	assert(pixels != NULL);

	image->width = width;
	image->height = height;
	image->stride = width*bpp;
	image->pixels = pixels;
};
static void image_save(const image_t *image, const char *file_name)
{
	const size_t bpp = 4;
	stbi_write_png("image.png",
		image->width, image->height, bpp,
		image->pixels, image->stride);
};

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

static bool sphere_hit(const sphere_t *sphere, ray_t ray, 
	f32 min_t, f32 max_t, hit_t *hit)
{
	const f32 eps = 1e-8;

	const v3 oc = v3_sub(ray.origin, sphere->center);
	const f32 a = v3_dot(ray.direction, ray.direction);
	const f32 b = v3_dot(ray.direction, oc);
	const f32 c = v3_dot(oc, oc) - (sphere->radius*sphere->radius);
	
	const f32 det = b*b - a*c;
	if (det >= eps)
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

static const bool world_hit(ray_t ray,
	f32 min_t, f32 max_t, hit_t *hit)
{
	sphere_t spheres[2];
	spheres[0].center = V3(0.f, 0.f, -1.f);
	spheres[0].radius = 0.5f;

	spheres[1].center = V3(0.f, 1000.5f, -1.f);
	spheres[1].radius = 1000.f;

	hit->t = INFINITY;

	bool result = false;
	for (u32 i = 0; i < static_len(spheres); i++)
	{
		const sphere_t *sphere = spheres + i;
		if (sphere_hit(sphere, ray, min_t, max_t, hit))
		{
			result = true;
		}
	};
	return result;
};
const v3 sample(ray_t ray)
{
	hit_t hit = {0};
	if (world_hit(ray, 0.f, FLT_MAX, &hit))
	{
		return v3_scale(v3_add(hit.normal, V3(1.f, 1.f, 1.f)), 0.5f);
	}
	return V3(0.f, 0.f, 0.f);
};

typedef struct
{
	v3 position;
	v3 x, y, z;
	v3 h, v, f;
} camera_t;

static camera_t look_at(
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

int main(int argc, const char *argv[])
{
	if (argc < 4)
	{
		printf("Usage: %s width height samples_per_pixel\n", argv[0]);
		return 0;
	}

	const i32 w = atoi(argv[1]);
	const i32 h = atoi(argv[2]);
	const i32 spp = atoi(argv[3]);

	const f32 fov = 75.f;
	const f32 aspect_ratio = ((f32) w / (f32) h);

	image_t image;
	image_alloc(&image, w, h);
	
	camera_t camera = look_at(
		V3(0.f, 0.f, 0.f),
		V3(0.f, 0.f,-1.f),
		V3(0.f, 1.f, 0.f),
		fov, aspect_ratio);

	u8 *row = image.pixels;
	for (u32 j = 0; j < image.height; j++)
	{
		u32 *pixel = (u32*) row;
		for (u32 i = 0; i < image.width; i++)
		{
			v3 color = V3(0.f, 0.f, 0.f);
			for (u32 s = 0; s < spp; s++)
			{
				const f32 u = (((f32) i + f32_rand()) / (f32) image.width);
				const f32 v = (((f32) j + f32_rand()) / (f32) image.height);

				ray_t ray = camera_ray(&camera, u, v);
				color = v3_add(color, sample(ray));
			}
			color = v3_scale(color, (1.f / (f32) spp));

			const u8 r = (u8) (color.r * 255.99f);
			const u8 g = (u8) (color.g * 255.99f);
			const u8 b = (u8) (color.b * 255.99f);
			*pixel++ = (0xFF << 24) | (b << 16) | (g << 8) | (r << 0);
		}
		row += image.stride;
	};
	image_save(&image, "image.png");
	return 0;
}