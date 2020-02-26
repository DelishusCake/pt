// Should tile-based rendering be used?
#define USE_TILES 1
// Should a BVH be used?
#define USE_BVH   1

#include "core.h"
#include "util.h"
#include "geom.h"

#include "image.h"
#include "scene.h"
#include "framebuffer.h"

#include "render.h"

#include <pthread.h>
#include <time.h>

#if USE_TILES
// Maximum number of tiles that can be rendered
// NOTE: Arbitrary, just used to keep the tile array length constant 
#define MAX_TILES			1024
// Maximum amount of memory that can be allocated from the tile scratch allocator
#define TILE_MEMORY_SIZE	kilobytes(16)

// Per-tile rendering data
typedef struct
{
	// Area to render for this tile
	rect_t area;
	// Scratch memory allocator for fast, thread-safe allocations
	lin_alloc_t temp_alloc;
} render_tile_t;
// Tile render queue
typedef struct
{
	// Input and output pointers
	scene_t *scene;
	framebuffer_t *framebuffer;
	// Total number of tiles to render
	u32 tile_count;
	// Tile array
	render_tile_t tiles[MAX_TILES];
	// Next tile to be rendered
	volatile u32 next_tile;
	// Number of completed tiles
	volatile u32 completed;
} render_queue_t;

// Renders a single tile, returns a boolean indicating if work was done
static bool render_tile(render_queue_t *queue)
{
	// If there's still work to be done
	if (queue->next_tile < queue->tile_count)
	{
		// Get the index of the next tile to render
		// NOTE: Done atomically so that no other thread can work on this tile
		const u32 index = atomic_inc(&queue->next_tile);
		// Alias some data to save typing
		rect_t area = queue->tiles[index].area;
		lin_alloc_t *temp_alloc = &queue->tiles[index].temp_alloc;
		// Call the render function on this tile
		render(temp_alloc,
			&queue->scene->world, 
			&queue->scene->camera,
			queue->scene->samples, 
			queue->scene->bounces, 
			queue->framebuffer, area);
		// Increment the completed count atomically
		atomic_inc(&queue->completed);
		return true;
	}
	return false;
}
static void* thread_proc(void *data)
{
	// Simple thread procedure to render tiles so long as some are available
	render_queue_t *queue = (render_queue_t*) data;
	while (render_tile(queue));
	// Exit the thread when no more work is available to be done
	return NULL;
};
static void render_tiles(scene_t *scene, framebuffer_t *framebuffer, u32 worker_count)
{
	// Make sure the scene fits in the render queue
	assert((scene->tiles_x*scene->tiles_y) <= MAX_TILES);
	// Get the dimensions of a tile in pixels
	const u32 tile_w = framebuffer->width / scene->tiles_x;
	const u32 tile_h = framebuffer->height / scene->tiles_y;
	// Allocate a render queue
	// NOTE: Needs to be heap allocated for thread safety
	render_queue_t *queue = malloc(sizeof(render_queue_t));
	assert(queue != NULL);
	memset(queue, 0, sizeof(render_queue_t));
	// Set the queue input/output data pointers
	queue->scene = scene;
	queue->framebuffer = framebuffer;
	// Set up each tile in the queue
	for (u32 j = 0; j < scene->tiles_y; j++)
	{
		for (u32 i = 0; i < scene->tiles_x; i++)
		{
			// Get the current tile
			render_tile_t *tile = queue->tiles + queue->tile_count++;
			// Set the tile dimensions
			tile->area.x = i*tile_w;
			tile->area.y = j*tile_h;
			tile->area.w = tile_w;
			tile->area.h = tile_h;
			// Allocate the tile scratch memory
			void *memory = malloc(TILE_MEMORY_SIZE);
			assert(memory != NULL);
			// Initialize the scratch memory allocator
			lin_alloc_init(&tile->temp_alloc, TILE_MEMORY_SIZE, memory);
		};
	};
	// Create a bunch of worker threads, each running the rendering function 
	const u32 thread_count = (worker_count-1);
	for (u32 i = 0; i < thread_count; i++)
	{
		pthread_t thread;
		pthread_create(&thread, NULL, thread_proc, queue);
	}
	// Keep doing jobs in the queue on the main thread too
	while (render_tile(queue));
	// Spinlock until all tiles are rendered
	// NOTE: Prevents an early exit when there's no more tiles in the queue BUT some tiles are still being rendered
	while (queue->completed < queue->tile_count)
		_mm_pause();
	// Free the tile memory
	for (u32 i = 0; i < queue->tile_count; i++)
	{
		render_tile_t *tile = queue->tiles + i;
		free(tile->temp_alloc.memory);
	}
	// Free the queue
	free(queue);
};
#endif

int main(int argc, const char *argv[])
{
	// Not enough command line arguments, early out with help message
	if (argc < 2)
	{
		printf("Usage: %s scene_file\n", argv[0]);
		return 0;
	}
	// Seed the RNG
	// TODO: Implement better, faster RNG
	srand(time(NULL));

	// Load the scene from a JSON file
	printf("Loading scene...");
	scene_t *scene = scene_load(argv[1]);
	if (scene)
	{
		printf("done\n");

		// Build the BVH for the world
		printf("Building bvh...");
		world_build_bvh(&scene->world);
		printf("done\n");

		framebuffer_t framebuffer;
		framebuffer_alloc(&framebuffer, scene->w, scene->h);
		
		// Begin rendering
		printf("Rendering...");
		{
			const clock_t start = clock();
			#if USE_TILES
				// Render using tile-based parallel method
				render_tiles(scene, &framebuffer, 8);
			#else
				// Render using a single core method
				// NOTE: Only use this as a benchmark!
				rect_t area = { 0,0,framebuffer.width,framebuffer.height};
				render(
					&scene->world, 
					&scene->camera,
					scene->samples, 
					scene->bounces, 
					&framebuffer, area);
			#endif
			// Output render time
			const clock_t end = clock();
			const double time = (double) (end - start) / CLOCKS_PER_SEC;
			printf("done\nRender took %f seconds\n", time);
		}

		#if 0
		draw_bvh(
			&scene->camera, 
			scene->world.bvh,
			&framebuffer);
		#endif

		// Allocate an output image
		image_t image;
		image_alloc(&image, scene->w, scene->h);
		// Resolve the framebuffer to the image
		printf("Storing framebuffer...");
		{
			const clock_t start = clock();
			
			framebuffer_resolve(&image, &framebuffer);
			
			const clock_t end = clock();
			const double time = (double) (end - start) / CLOCKS_PER_SEC;
			printf("done\nStore took %f seconds\n", time);
		}
		image_save(&image, scene->output);
		// Cleanup
		framebuffer_free(&framebuffer);
		image_free(&image);
		free(scene);
	} else printf("Failed to load scene \"%s\"", argv[1]);
	return 0;
}