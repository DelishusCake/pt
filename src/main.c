#include "core.h"
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
} hit_t;

typedef struct
{
	v3  center;
	f32 radius;
} sphere_t;

static hit_t sphere_hit(sphere_t sphere, ray_t ray)
{
	const f32 eps = 1e-5;

	const v3  oc = v3_sub(ray.origin, sphere.center);
	const f32 a = v3_dot(ray.direction, ray.direction);
	const f32 b = 2.f * v3_dot(oc, ray.direction);
	const f32 c = v3_dot(oc, oc) - (sphere.radius*sphere.radius);
	const f32 det = b*b - 4.f*a*c;
	
	hit_t hit;
	if (det >= eps)
	{
		hit.t = (-b - f32_sqrt(det)) / (2.f*a);
	} else {
		hit.t = 0;
	}
	return hit;
};

const v3 sample(ray_t ray)
{
	sphere_t sphere;
	sphere.center = V3(0.f, -1.f, 0.f);
	sphere.radius = 0.5f;

	hit_t hit = sphere_hit(sphere, ray);
	if (hit.t > 0)
	{
		return V3(1.f, 0.f, 0.f);
	}
	return V3(0.f, 0.f, 0.f);
};

typedef struct
{
	v3 position;
	v3 x, y, z;
} camera_t;

static camera_t look_at(v3 position, v3 at, v3 up)
{
	v3 z = v3_norm(v3_sub(position, at));
	v3 x = v3_norm(v3_cross(up, z));
	v3 y = v3_norm(v3_cross(z, x));

	camera_t camera;
	camera.position = position;
	camera.x = x;
	camera.y = y;
	camera.z = z;
	return camera;
};
static ray_t camera_ray(const camera_t *camera, const image_t *image, f32 u, f32 v)
{
	const f32 film_width = 1.f;
	const f32 film_height = ((f32) image->height / (f32) image->width);
	const f32 film_distance = 0.25f;	
	
	const f32 offset_x = (/*(2.f*randf() - 1.f)*/0.f * 0.5f/(f32)image->width) + u;
	const f32 offset_y = (/*(2.f*randf() - 1.f)*/0.f * 0.5f/(f32)image->height) + v;

	const v3 film_center = v3_sub(camera->position, v3_scale(camera->z, film_distance));
	const v3 film_position = v3_add(film_center, v3_add(
		v3_scale(camera->x, offset_x*0.5f*film_width), 
		v3_scale(camera->y, offset_y*0.5f*film_height)
	));

	ray_t ray;
	ray.origin = camera->position;
	ray.direction = v3_norm(v3_sub(film_position, camera->position));
	return ray;
}

int main(int argc, const char *argv[])
{
	image_t image;
	image_alloc(&image, 1920 >> 1, 1080 >> 1);
	
	camera_t camera = look_at(
		V3(0.f, 0.f, 0.f),
		V3(0.f,-1.f, 0.f),
		V3(0.f, 0.f, 1.f)
	);

	u8 *row = image.pixels;
	for (u32 y = 0; y < image.height; y++)
	{
		u32 *pixel = (u32*) row;
		for (u32 x = 0; x < image.width; x++)
		{
			const f32 u = 2.f * ((f32) x / (f32) image.width) - 1.f;
			const f32 v = 2.f * ((f32) y / (f32) image.height) - 1.f;

			ray_t ray = camera_ray(&camera, &image, u, v);

			v3 color = sample(ray);

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