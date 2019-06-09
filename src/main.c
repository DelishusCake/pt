#include "core.h"
#include "util.h"
#include "geom.h"

#include <jsmn.h>
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

#define MAX_SPHERES	256

typedef struct
{
	u32 sphere_count;
	sphere_t spheres[MAX_SPHERES];
} world_t;

static const bool world_hit(const world_t *world, ray_t ray,
	f32 min_t, f32 max_t, hit_t *hit)
{
	hit->t = INFINITY;

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
const v3 render_sample(const world_t *world, ray_t ray)
{
	const f32 min_t = 0.f;
	const f32 max_t = FLT_MAX;

	hit_t hit = {0};
	if (world_hit(world, ray, min_t, max_t, &hit))
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

typedef struct
{
	i32 w, h, spp;
	camera_t camera;
	world_t world;
} scene_t;

static void scene_parse_image(scene_t *scene, 
	const char *code,
	const jsmntok_t *tokens, i32 token_count, i32 *current_token)
{
	const jsmntok_t *top = tokens + *current_token;
	for (u32 i = 0; i < top->size; i++)
	{
		const jsmntok_t *name = (tokens + (*current_token + 1));
		const jsmntok_t *value = (tokens + (*current_token + 2));
		*current_token += 2;

		const size_t value_len = (value->end-value->start);
		
		char val_str[value_len+1];
		memcpy(val_str, (code + value->start), value_len);
		val_str[value_len] = '\0';

		const char  *name_str = (code + name->start);
		const size_t name_len = (name->end-name->start);

		if (strncmp("width", name_str, name_len))  scene->w = atoi(val_str);
		if (strncmp("height", name_str, name_len)) scene->h = atoi(val_str);
		if (strncmp("spp", name_str, name_len))    scene->spp = atoi(val_str);
	};
};
static void scene_parse_camera(scene_t *scene, 
	const char *code,
	const jsmntok_t *tokens, i32 token_count, i32 *current_token)
{
	const jsmntok_t *top = tokens + *current_token;
	#if 0
	for (u32 i = 0; i < top->size; i++)
	{
		const jsmntok_t *name = (tokens + (*current_token + 1));
		const jsmntok_t *value = (tokens + (*current_token + 2));
		*current_token += 2;

		const size_t value_len = (value->end-value->start);
		
		char val_str[value_len+1];
		memcpy(val_str, (code + value->start), value_len);
		val_str[value_len] = '\0';

		const char  *name_str = (code + name->start);
		const size_t name_len = (name->end-name->start);
	};
	#endif
};
static void scene_parse(scene_t *scene, 
	const char *code,
	const jsmntok_t *tokens, i32 token_count)
{
	i32 current_token = 0;
	assert(tokens[current_token++].type == JSMN_OBJECT);

	while (current_token < token_count)
	{
		const jsmntok_t *token = tokens + current_token++;
		
		const char  *str = (code + token->start);
		const size_t len = (token->end-token->start);
		if (strncmp("image", str, len) == 0)
			scene_parse_image(scene, code, tokens, token_count, &current_token);
		if (strncmp("camera", str, len) == 0)
			scene_parse_camera(scene, code, tokens, token_count, &current_token);
	};
};
static scene_t* load_scene(const char *file_name)
{
	scene_t *scene = NULL;

	size_t len = 0;
	char *code = load_entire_file(file_name, &len);
	if (code)
	{
		jsmn_parser parser;
		jsmn_init(&parser);

		jsmntok_t tokens[256];
		i32 token_count = jsmn_parse(&parser, 
			code, len, 
			tokens, static_len(tokens));
		if (token_count > 0)
		{
			scene = malloc(sizeof(scene_t));
			assert(scene != NULL);
			memset(scene, 0, sizeof(scene_t));

			scene_parse(scene, code, tokens, token_count);
		};
	};
	return scene;
};

int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s scene_file\n", argv[0]);
		return 0;
	}

	scene_t *scene = load_scene(argv[1]);
	if (scene)
	{
		printf("Image: %dx%d %dspp\n", scene->w, scene->h, scene->spp);

		#if 0
		image_t image;
		image_alloc(&image, scene->w, scene->h);
		
		u8 *row = image.pixels;
		for (u32 j = 0; j < image.height; j++)
		{
			u32 *pixel = (u32*) row;
			for (u32 i = 0; i < image.width; i++)
			{
				v3 color = V3(0.f, 0.f, 0.f);
				for (u32 s = 0; s < scene->spp; s++)
				{
					const f32 u = (((f32) i + f32_rand()) / (f32) image.width);
					const f32 v = (((f32) j + f32_rand()) / (f32) image.height);

					ray_t ray = camera_ray(&scene->camera, u, v);
					color = v3_add(color, render_sample(&scene->world, ray));
				}
				color = v3_scale(color, (1.f / (f32) scene->spp));

				const u8 r = (u8) (color.r * 255.99f);
				const u8 g = (u8) (color.g * 255.99f);
				const u8 b = (u8) (color.b * 255.99f);
				*pixel++ = (0xFF << 24) | (b << 16) | (g << 8) | (r << 0);
			}
			row += image.stride;
		};
		image_save(&image, "image.png");
		#endif
		free(scene);
	}
	return 0;

	#if 0
	const f32 fov = 75.f;
	const f32 aspect_ratio = ((f32) w / (f32) h);

	camera_t camera = look_at(
		V3(0.f, 0.f, 0.f),
		V3(0.f, 0.f,-1.f),
		V3(0.f, 1.f, 0.f),
		fov, aspect_ratio);

	world_t *world = malloc(sizeof(world_t));
	assert(world != NULL);
	memset(world, 0, sizeof(world_t));

	world->spheres[0].center = V3(0.f, 0.f, -1.f);
	world->spheres[0].radius = 0.5f;

	world->spheres[1].center = V3(0.f, 1000.5f, -1.f);
	world->spheres[1].radius = 1000.f;

	world->sphere_count = 2;

	
	return 0;
	#endif
}