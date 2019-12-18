// Should tile-based rendering be used?
#define USE_TILES 1
// Should a BVH be used?
#define USE_BVH   1

#include "core.h"
#include "util.h"
#include "geom.h"

#include "image.h"
#include "scene.h"
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

static inline u32 srgb(v3 color)
{
	const f32 exp = (1.f/2.2f);
	const u8 a = 0xFF;
	const u8 r = (u32)(f32_pow(f32_saturate(color.r), exp) * 255.f + 0.5f) & 0xFF;
	const u8 g = (u32)(f32_pow(f32_saturate(color.g), exp) * 255.f + 0.5f) & 0xFF;
	const u8 b = (u32)(f32_pow(f32_saturate(color.b), exp) * 255.f + 0.5f) & 0xFF;
	return (a << 24) | (b << 16) | (g << 8) | (r << 0);
};
static void framebuffer_store(image_t *image, const framebuffer_t *framebuffer)
{
	// Get a pointer to the begining of the render area
	u8 *row = image->pixels;
	// For each row of the area to render
	for (u32 j = 0; j < image->height; j++)
	{
		// Iterate over each pixel
		u32 *pixel = (u32*) row;
		for (u32 i = 0; i < image->width; i++)
		{
			// Get the color
			const v3 color = framebuffer->pixels[j*framebuffer->width + i];
			// Store the pixel in srgb space
			*pixel++ = srgb(color);
		}
		// Iterate to the next row
		row += image->stride;
	};
};

#if 1
static void framebuffer_filter(image_t *image, const framebuffer_t *framebuffer)
{
	const i32 kernelSize = 3;
	const f32 id = 5000.f;
	const f32 cd = 2.f;

	u8 *row = image->pixels;
	for (u32 y = 0; y < framebuffer->height; y++)
	{
		// Iterate over each pixel
		u32 *pixel = (u32*) row;
		for (u32 x = 0; x < framebuffer->width; x++)
		{
			// Get the color
			const v3 color = framebuffer->pixels[y*framebuffer->width + x];

			i32 k_min_x = x - kernelSize;
			i32 k_min_y = y - kernelSize;

			i32 k_max_x = x + kernelSize;
			i32 k_max_y = y + kernelSize;

			f32 s_weight = 0.f;
			v3 s_color = V3(0.f, 0.f, 0.f);
			for (u32 j = k_min_y; j <= k_max_y; j++)
			{
				for (u32 i = k_min_x; i <= k_max_x; i++)
				{
					const i32 n_x = clamp(i, 0, (framebuffer->width-1));
					const i32 n_y = clamp(j, 0, (framebuffer->height-1));

					const v3 n_color = framebuffer->pixels[n_y*framebuffer->width + n_x];

					f32 image_dist = f32_sqrt((f32) ((i-x)*(i-x)) + (f32) ((j-y)*(j-y)));
					f32 color_dist = f32_sqrt((f32) (
						((color.r - n_color.r)*(color.r - n_color.r)) +
						((color.g - n_color.g)*(color.g - n_color.g)) +
						((color.b - n_color.b)*(color.b - n_color.b))));
					f32 weight = 1.f / (f32_exp((image_dist/id)*(image_dist/id)*0.5f) * f32_exp((color_dist/cd)*(color_dist/cd)*0.5f));
					s_weight += weight;

					s_color.r += weight*color.r;
					s_color.g += weight*color.g;
					s_color.b += weight*color.b;
				}
			}
			s_color.r *= (1.f / s_weight);
			s_color.g *= (1.f / s_weight);
			s_color.b *= (1.f / s_weight);

			// Store the pixel in srgb space
			*pixel++ = srgb(s_color);
		}
		// Iterate to the next row
		row += image->stride;
	};
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
		// Store the framebuffer to the image
		printf("Storing framebuffer...");
		{
			const clock_t start = clock();
			framebuffer_store(&image, &framebuffer);
			// framebuffer_filter(&image, &framebuffer);
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