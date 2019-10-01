#ifndef CORE_H
#define CORE_H

#if defined(__GNUC__)
#define GCC 1
#else
#define GCC 0
#endif
#if defined(_MSC_VER)
#define MSVC 1
#else
#define MSVC 0
#endif

#include <assert.h>

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <string.h>
#include <float.h>
#include <math.h>

#include <xmmintrin.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;

#define kilobytes(n)	((n)<<10)
#define megabytes(n)	((n)<<20)
#define gigabytes(n)	((n)<<30)

#define PI_32			(3.14159f)

#define to_radians(f)	((f) * (PI_32 / 180.f))
#define to_degrees(f)	((f) * (180.f / PI_32))

#define sign(v)	(((v) < 0) ? -1 : (((v) > 0) ? 1 : 0))

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#define clamp(v, l, h)	max(l, min(v, h))

#define stringify(str)	#str

#define swap(type, a, b) { type tmp = (a); (a) = (b); (b) = tmp; }

#define static_len(a)	          (sizeof(a)/sizeof(a[0]))
#define member_size(type, member) (sizeof(((type *)0)->member))
#define decl_struct(name)         struct name; typedef struct name name;

#define heap_parent(i)	((i - 1) >> 1)
#define heap_left(i)	((i << 1) + 1)
#define heap_right(i)	((i << 1) + 2)

// TODO: Implement these as intrinsics
inline f32 f32_ceil(f32 v)        { return ceilf(v); };
inline f32 f32_floor(f32 v)       { return floorf(v); };
inline f32 f32_round(f32 v)       { return roundf(v); };

inline f32 f32_abs(f32 v)         { return fabsf(v); };
inline f32 f32_sqrt(f32 v)        { return sqrtf(v); };
inline f32 f32_pow(f32 v, f32 p)  { return powf(v, p); };
inline f32 f32_isqrt(f32 v)       { return (1.f / sqrtf(v)); };
inline f32 f32_square(f32 v)      { return v*v; };

inline f32 f32_sin(f32 v)         { return sinf(v); }
inline f32 f32_cos(f32 v)         { return cosf(v); }
inline f32 f32_tan(f32 v)         { return tanf(v); }
inline f32 f32_atan(f32 v)        { return atanf(v); }

inline f32 f32_saturate(f32 v)    { return clamp(v, 0.f, 1.f); }

inline u32 atomic_inc(volatile u32 *value)
{
#if GCC
	return __sync_fetch_and_add(value, 1);
#elif MSVC
	return _InterlockedExchangeAdd(value, 1);
#endif
}

#endif