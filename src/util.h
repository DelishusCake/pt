#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#include "core.h"
#include "geom.h"

// rand range [0.f, 1.f]
f32 f32_rand();

v3 v3_unit_rand();

char* load_entire_file(const char *file_name, size_t *size);

typedef struct
{
	size_t  used;
	size_t  size;
	void   *memory;
} lin_alloc_t;

void  lin_alloc_init(lin_alloc_t *lin_alloc, size_t size, void *memory);
void* lin_alloc_push(lin_alloc_t *lin_alloc, size_t size);
void  lin_alloc_reset(lin_alloc_t *lin_alloc);

#endif