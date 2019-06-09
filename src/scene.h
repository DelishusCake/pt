#ifndef SCENE_H
#define SCENE_H

#include "core.h"
#include "util.h"

#include "render.h"

typedef struct
{
	i32 w, h, spp;
	char output[512];

	world_t world;
	camera_t camera;
} scene_t;

scene_t* scene_load(const char *file_name);

#endif