#include "util.h"

f32 f32_rand()
{
	return ((f32) rand()/(f32) (RAND_MAX));
}
u32 u32_rand(u32 lo, u32 hi)
{
	return (rand() % (hi - lo + 1)) + lo; 
};
v2 v2_unit_rand()
{
	v2 p;
	do 
	{
		v2 r = V2(f32_rand(), f32_rand());
		p = v2_sub(v2_scale(r, 2.f), V2(1.f, 1.f)); 
	} while (v2_len2(p) >= 1.f);
	return p;
};
v3 v3_unit_rand()
{
	v3 p;
	do 
	{
		v3 r = V3(f32_rand(), f32_rand(), f32_rand());
		p = v3_sub(v3_scale(r, 2.f), V3(1.f, 1.f, 1.f)); 
	} while (v3_len2(p) >= 1.f);
	return p;
};

char* load_entire_file(const char *file_name, size_t *size)
{
	char *buffer = NULL;

	FILE *f = fopen(file_name, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		const size_t f_size = ftell(f);
		fseek(f, 0, SEEK_SET);

		buffer = malloc((f_size+1)*sizeof(char));
		fread(buffer, sizeof(char), f_size, f);
		buffer[f_size] = '\0';

		if (size) *size = f_size;
	};
	return buffer;
};

void lin_alloc_init(lin_alloc_t *lin_alloc, size_t size, void *memory)
{
	lin_alloc->used = 0;
	lin_alloc->size = size;
	lin_alloc->memory = memory;
};
void lin_alloc_reset(lin_alloc_t *lin_alloc)
{
	lin_alloc->used = 0;
};
void* lin_alloc_push(lin_alloc_t *lin_alloc, size_t size)
{
	void *ptr = NULL;
	if ((lin_alloc->used + size) < lin_alloc->size)
	{
		ptr = (uint8_t*) lin_alloc->memory + lin_alloc->used;
		lin_alloc->used += size;
	}
	return ptr;
};