#ifndef SCENE_H
#define SCENE_H

#include "core.h"
#include "util.h"

#include "render.h"

typedef struct
{
	// Image 
	i32 w, h;
	char output[512];
	// Render 
	i32 samples, bounces;
	i32 tiles_x, tiles_y;
	// World
	world_t world;
	camera_t camera;
} scene_t;

scene_t* scene_load(const char *file_name);

#endif