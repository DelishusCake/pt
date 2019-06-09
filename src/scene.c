#include "scene.h"

#include <jsmn.h>

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
static void parser_get_str(const parser_t *parser, const jsmntok_t *token, 
	char *str, size_t str_len)
{
	assert (token->type == JSMN_STRING);

	const size_t len = (token->end-token->start);
	const char  *src = (parser->string + token->start);

	assert((len+1) <= str_len);
	memcpy(str, src, len);
	str[len] = '\0';
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

		if (parser_check_equals(parser, name, "width"))  scene->w = parser_get_i32(parser, value);
		if (parser_check_equals(parser, name, "height")) scene->h = parser_get_i32(parser, value);
		if (parser_check_equals(parser, name, "spp"))    scene->spp = parser_get_i32(parser, value);
		if (parser_check_equals(parser, name, "name"))   parser_get_str(parser, value, scene->output, static_len(scene->output));
	};

	#if 0
	printf("IMAGE: \"%s\" %dx%d %d SPP\n", scene->output, scene->w, scene->h, scene->spp);
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
scene_t* scene_load(const char *file_name)
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