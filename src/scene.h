#ifndef SCENE_H
#define SCENE_H

#include "core.h"
#include "util.h"

#include "world.h"

typedef struct
{
	// Image data
	i32 w, h;
	char output[512];
	// Render data
	i32 samples, bounces;
	i32 tiles_x, tiles_y;
	// World data
	world_t world;
	camera_t camera;
} scene_t;

// Load a scene from a JSON file
scene_t* scene_load(const char *file_name);

#endif