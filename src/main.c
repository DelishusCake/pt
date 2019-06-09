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

#define MAX_SPHERES	256

typedef struct
{
	u32 sphere_count;
	sphere_t spheres[MAX_SPHERES];
} world_t;

static const bool world_hit(const world_t *world, ray_t ray,
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
const v3 render_sample(const world_t *world, ray_t ray)
{
	const f32 min_t = 1e-8;
	const f32 max_t = FLT_MAX;

	hit_t hit;
	hit.t = INFINITY;
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


typedef struct
{
	const char *string;
	
	i32 token_count;
	i32 current_token;
	jsmntok_t tokens[256];
} parser_t;

static const jsmntok_t* parser_get(parser_t *parser)
{
	return parser->tokens + parser->current_token++;
};

static bool parser_check_equals(const parser_t *parser, const jsmntok_t *token, const char *check)
{
	const char  *tok_str = (parser->string + token->start);
	const size_t tok_len = (token->end-token->start);
	return (strncmp(check, tok_str, tok_len) == 0);
};
static i32 parser_get_i32(const parser_t *parser, const jsmntok_t *token)
{
	assert(token->type == JSMN_PRIMITIVE);
	const size_t len = (token->end-token->start);
		
	char str[len+1];
	memcpy(str, (parser->string + token->start), len);
	str[len] = '\0';

	return atoi(str);
};
static f32 parser_get_f32(const parser_t *parser, const jsmntok_t *token)
{
	assert(token->type == JSMN_PRIMITIVE);
	const size_t len = (token->end-token->start);
		
	char str[len+1];
	memcpy(str, (parser->string + token->start), len);
	str[len] = '\0';

	return atof(str);
};

static v3 parser_get_v3(parser_t *parser, const jsmntok_t *token)
{
	assert(token->type == JSMN_ARRAY);
	assert(token->size == 3);

	v3 v;
	v.x = parser_get_f32(parser, parser_get(parser));
	v.y = parser_get_f32(parser, parser_get(parser));
	v.z = parser_get_f32(parser, parser_get(parser));
	return v;
};

static void scene_parse_image(scene_t *scene, parser_t *parser)
{
	const jsmntok_t *top = parser_get(parser);
	assert(top->type == JSMN_OBJECT);

	for (u32 i = 0; i < top->size; i++)
	{
		const jsmntok_t *name = parser_get(parser);
		const jsmntok_t *value = parser_get(parser);

		if (parser_check_equals(parser, name, "width"))
			scene->w = parser_get_i32(parser, value);
		if (parser_check_equals(parser, name, "height"))
			scene->h = parser_get_i32(parser, value);
		if (parser_check_equals(parser, name, "spp"))
			scene->spp = parser_get_i32(parser, value);
	};

	#if 0
	printf("IMAGE: %dx%d %d SPP\n", scene->w, scene->h, scene->spp);
	#endif
};
static void scene_parse_camera(scene_t *scene, parser_t *parser)
{
	f32 fov = 0.f;
	f32 aspect_ratio = 0.f;
	v3 up = V3(0.f, 0.f, 0.f);
	v3 at = V3(0.f, 0.f, 0.f);
	v3 position = V3(0.f, 0.f, 0.f);

	const jsmntok_t *top = parser_get(parser);
	assert(top->type == JSMN_OBJECT);

	for (u32 i = 0; i < top->size; i++)
	{
		const jsmntok_t *name = parser_get(parser);
		const jsmntok_t *value = parser_get(parser);

		if (parser_check_equals(parser, name, "fov"))
			fov = parser_get_f32(parser, value);
		if (parser_check_equals(parser, name, "aspect_ratio"))
			aspect_ratio = parser_get_f32(parser, value);
		if (parser_check_equals(parser, name, "position"))
			position = parser_get_v3(parser, value);
		if (parser_check_equals(parser, name, "up"))
			up = parser_get_v3(parser, value);
		if (parser_check_equals(parser, name, "at"))
			at = parser_get_v3(parser, value);
	};

	#if 0
	printf("FOV: %f\n", fov);
	printf("ASP: %f\n", aspect_ratio);
	printf("POS: %f, %f, %f\n", position.x, position.y, position.z);
	printf("AT : %f, %f, %f\n", at.x, at.y, at.z);
	printf("UP : %f, %f, %f\n", up.x, up.y, up.z);
	#endif

	scene->camera = look_at(
		position, at, up,
		fov, aspect_ratio);
};
static void scene_parse_sphere(scene_t *scene, parser_t *parser)
{
	f32 radius = 0.f;
	v3 center = V3(0.f, 0.f, 0.f);

	const jsmntok_t *top = parser_get(parser);
	assert(top->type == JSMN_OBJECT);

	for (u32 i = 0; i < top->size; i++)
	{
		const jsmntok_t *name = parser_get(parser);
		const jsmntok_t *value = parser_get(parser);

		if (parser_check_equals(parser, name, "center"))
			center = parser_get_v3(parser, value);
		if (parser_check_equals(parser, name, "radius"))
			radius = parser_get_f32(parser, value);
	};

	#if 0
	printf("RADIUS: %f\n", radius);
	printf("CENTER: %f, %f, %f\n", center.x, center.y, center.z);
	#endif

	assert((scene->world.sphere_count + 1) < MAX_SPHERES);

	const u32 index = scene->world.sphere_count++;
	scene->world.spheres[index].radius = radius;
	scene->world.spheres[index].center = center;
};
static void scene_parse(scene_t *scene, parser_t *parser)
{
	const jsmntok_t *top = parser_get(parser);
	assert(top->type == JSMN_OBJECT);

	for (u32 i = 0; i < top->size; i++)
	{
		const jsmntok_t *token = parser_get(parser);
		if (parser_check_equals(parser, token, "image"))
			scene_parse_image(scene, parser);
		if (parser_check_equals(parser, token, "camera"))
			scene_parse_camera(scene, parser);
		if (parser_check_equals(parser, token, "sphere"))
			scene_parse_sphere(scene, parser);
	};
};
static scene_t* scene_load(const char *file_name)
{
	scene_t *scene = NULL;

	size_t len = 0;
	char *code = load_entire_file(file_name, &len);
	if (code)
	{
		jsmn_parser parser;
		jsmn_init(&parser);

		parser_t p;
		p.string = code;
		p.current_token = 0;

		p.token_count = jsmn_parse(&parser, 
			p.string, len, 
			p.tokens, static_len(p.tokens));
		if (p.token_count > 0)
		{
			scene = malloc(sizeof(scene_t));
			assert(scene != NULL);
			memset(scene, 0, sizeof(scene_t));

			scene_parse(scene, &p);
		};
	};
	return scene;
};
static void scene_render(const scene_t *scene, image_t *image)
{
	image_alloc(image, scene->w, scene->h);

	u8 *row = image->pixels;
	for (u32 j = 0; j < image->height; j++)
	{
		u32 *pixel = (u32*) row;
		for (u32 i = 0; i < image->width; i++)
		{
			v3 color = V3(0.f, 0.f, 0.f);
			for (u32 s = 0; s < scene->spp; s++)
			{
				const f32 u = (((f32) i + f32_rand()) / (f32) image->width);
				const f32 v = (((f32) j + f32_rand()) / (f32) image->height);

				ray_t ray = camera_ray(&scene->camera, u, v);
				color = v3_add(color, render_sample(&scene->world, ray));
			}
			color = v3_scale(color, (1.f / (f32) scene->spp));

			const u8 r = (u8) (color.r * 255.99f);
			const u8 g = (u8) (color.g * 255.99f);
			const u8 b = (u8) (color.b * 255.99f);
			*pixel++ = (0xFF << 24) | (b << 16) | (g << 8) | (r << 0);
		}
		row += image->stride;
	};
};

int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s scene_file\n", argv[0]);
		return 0;
	}

	scene_t *scene = scene_load(argv[1]);
	if (scene)
	{
		image_t image;
		scene_render(scene, &image);
		image_save(&image, "image.png");
		free(scene);
	}
	return 0;
}