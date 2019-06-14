#include "core.h"
#include "util.h"
#include "geom.h"

#include "image.h"
#include "scene.h"
#include "render.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <time.h>

#define USE_TILES 1

#if USE_TILES
#define MAX_TILES	1024

typedef struct
{
	scene_t *scene;
	image_t *image;

	u32 tile_count;
	rect_t tiles[MAX_TILES];
	
	volatile u32 next_tile;
	volatile u32 completed;
} render_queue_t;

static bool render_tile(render_queue_t *queue)
{
	if(queue->next_tile < queue->tile_count)
	{
		const u32 index = atomic_inc(&queue->next_tile);

		rect_t tile = queue->tiles[index];
		render(
			&queue->scene->world, 
			&queue->scene->camera,
			queue->scene->samples, 
			queue->scene->bounces, 
			queue->image, tile);

		atomic_inc(&queue->completed);
		return true;
	}
	return false;
}
static DWORD WINAPI thread_proc(void *data)
{
	render_queue_t *queue = (render_queue_t*)data;
	while(render_tile(queue));
	return 0;
};
static void render_tiles(scene_t *scene, image_t *image, u32 worker_count)
{
	assert((scene->tiles_x*scene->tiles_y) <= MAX_TILES);

	const u32 tile_w = image->width / scene->tiles_x;
	const u32 tile_h = image->height / scene->tiles_y;

	render_queue_t *queue = malloc(sizeof(render_queue_t));
	assert(queue != NULL);
	memset(queue, 0, sizeof(render_queue_t));

	queue->scene = scene;
	queue->image = image;

	for (u32 j = 0; j < scene->tiles_y; j++)
	{
		for (u32 i = 0; i < scene->tiles_x; i++)
		{
			rect_t tile;
			tile.x = i*tile_w;
			tile.y = j*tile_h;
			tile.w = tile_w;
			tile.h = tile_h;

			queue->tiles[queue->tile_count++] = tile;
		};
	};

	for(u32 i = 0; i < (worker_count-1); i++)
	{
		DWORD dwThreadID = 0;
		HANDLE hThread = CreateThread(NULL, 0, thread_proc, queue, 0, &dwThreadID);
		CloseHandle(hThread);
	}

	while (render_tile(queue));

	while (queue->completed < queue->tile_count)
		_mm_pause();

	free(queue);
};
#endif

int main(int argc, const char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s scene_file\n", argv[0]);
		return 0;
	}

	srand(time(NULL));

	printf("Loading scene...");
	scene_t *scene = scene_load(argv[1]);
	if (scene)
	{
		printf("done\n");

		printf("Building bvh...");
		world_build_bvh(&scene->world);
		printf("done\n");

		image_t image;
		image_alloc(&image, scene->w, scene->h);

		printf("Rendering...");
		const clock_t start = clock();
		#if USE_TILES
			render_tiles(scene, &image, 8);
		#else
			rect_t area = { 0,0,image.width,image.height};
			render(
				&scene->world, 
				&scene->camera,
				scene->samples, 
				scene->bounces, 
				&image, area);
		#endif
		const clock_t end = clock();
		const double time = (double) (end - start) / CLOCKS_PER_SEC;
		printf("done\nRender took %f seconds\n", time);

		image_save(&image, scene->output);
		image_free(&image);
		free(scene);
	}
	return 0;
}