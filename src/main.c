#include "core.h"
#include "util.h"
#include "geom.h"

#include "image.h"
#include "scene.h"
#include "render.h"

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
		image_alloc(&image, scene->w, scene->h);

		rect_t area = { 0,0,image.width,image.height};
		render(
			&scene->world, 
			&scene->camera,
			scene->spp, 
			&image, area);

		image_save(&image, scene->output);
		image_free(&image);
		free(scene);
	}
	return 0;
}