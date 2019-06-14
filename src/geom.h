#ifndef GEOM_H
#define GEOM_H

#include "core.h"

typedef struct
{
	f32 x, y;
} v2;

typedef union
{
	struct { f32 x, y, z; };
	struct { f32 r, g, b; };
} v3;

typedef union
{
	struct { f32 x, y, z, w; };
	struct { f32 r, g, b, a; };
} v4;

typedef union
{
	struct
	{
		f32 x0, y0, z0, w0;
		f32 x1, y1, z1, w1;
		f32 x2, y2, z2, w2;
		f32 x3, y3, z3, w3;
	};
	f32 m[4][4];
} m44;

typedef struct
{
	v3 origin;
	v3 direction;
} ray_t;

typedef struct
{
	i32 x, y, w, h;
} rect_t;

/* V2 */
inline v2 V2(f32 x, f32 y)
{
	v2 v;
	v.x = x;
	v.y = y;
	return v;
}
inline v2  v2_add(v2 a, v2 b)		{ return V2(a.x+b.x, a.y+b.y); };
inline v2  v2_sub(v2 a, v2 b)  		{ return V2(a.x-b.x, a.y-b.y); };
inline v2  v2_mul(v2 a, v2 b)  		{ return V2(a.x*b.x, a.y*b.y); };
inline v2  v2_scale(v2 v, f32 s)	{ return V2(v.x*s, v.y*s); };
inline v2  v2_neg(v2 v)  			{ return V2(-v.x, -v.y); };
inline v2  v2_perp(v2 v)			{ return V2(v.y, -v.x); }
inline f32 v2_cross(v2 a, v2 b)		{ return a.x*b.y - a.y*b.x; };
inline f32 v2_dot(v2 a, v2 b)		{ return a.x*b.x + a.y*b.y; };
inline f32 v2_len2(v2 v)			{ return v2_dot(v, v); };
inline v2  v2_norm(v2 v)
{
	const f32 l2 = v2_len2(v);
	if(l2 > 1e-8f)
		return v2_scale(v, f32_isqrt(l2));
	return v;
}

/* V3 */
inline v3 V3(f32 x, f32 y, f32 z)
{
	v3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}
inline v3  v3_add(v3 a, v3 b)		{ return V3(a.x+b.x, a.y+b.y, a.z+b.z); };
inline v3  v3_sub(v3 a, v3 b)  		{ return V3(a.x-b.x, a.y-b.y, a.z-b.z); };
inline v3  v3_mul(v3 a, v3 b)  		{ return V3(a.x*b.x, a.y*b.y, a.z*b.z); };
inline v3  v3_scale(v3 v, f32 s)	{ return V3(v.x*s, v.y*s, v.z*s); };
inline v3  v3_neg(v3 v)  			{ return V3(-v.x, -v.y, -v.z); };
inline f32 v3_dot(v3 a, v3 b)		{ return a.x*b.x + a.y*b.y + a.z*b.z; };
inline v3  v3_cross(v3 a, v3 b)
{
	v3 r;
	r.x = a.y*b.z - a.z*b.y;
	r.y = a.z*b.x - a.x*b.z;
	r.z = a.x*b.y - a.y*b.x;
	return r;
};
inline f32 v3_len2(v3 v)			{ return v3_dot(v, v); };
inline f32 v3_len(v3 v)				{ return f32_sqrt(v3_len2(v)); };
inline v3  v3_norm(v3 v)
{
	const f32 l2 = v3_len2(v);
	if(l2 > 1e-8f)
		return v3_scale(v, f32_isqrt(l2));
	return v;
}
inline v3 v3_refl(v3 v, v3 n)
{
	return v3_sub(v, v3_scale(n, 2.f*v3_dot(v, n)));
}

/* V4 */
inline v4 V4(f32 x, f32 y, f32 z, f32 w)
{
	v4 v;
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return v;
}
inline v4  v4_scale(v4 v, f32 s)	{ return V4(v.x*s, v.y*s, v.z*s, v.w*s); };

/* M44 */
inline m44 m44_identity()
{
	return (m44)
	{{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f,
	}};
};
inline m44 m44_scale(f32 x, f32 y, f32 z)
{
	return (m44)
	{{
		x,   0.f, 0.f, 0.f,
		0.f, y,   0.f, 0.f,
		0.f, 0.f, z,   0.f,
		0.f, 0.f, 0.f, 1.f,
	}};
};
inline m44 m44_rotationZ(f32 theta)
{
	const f32 c = f32_cos(theta);
	const f32 s = f32_sin(theta);
	return (m44)
	{{
		c,  -s,   0.f, 0.f,
		s,   c,   0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f,
	}};
};
inline m44 m44_translation(f32 x, f32 y, f32 z)
{
	return (m44)
	{{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		x,   y,   z,   1.f,
	}};
};
inline m44 m44_orthoOffCenter(f32 l, f32 r, f32 b, f32 t, f32 zn, f32 zf)
{
	const f32 sx = (2.f / (r-l));
	const f32 sy = (2.f / (t-b));
	const f32 sz = (1.f / (zf-zn));

	const f32 tx = (l+r)/(l-r);
	const f32 ty = (t+b)/(b-t);
	const f32 tz = zn / (zn - zf);

	return (m44)
	{{
		sx, 0.f, 0.f, 0.f,
		0.f, sy, 0.f, 0.f,
		0.f, 0.f, sz, 0.f,
		tx,  ty,  tz, 1.f,
	}};
};
inline m44 m44_mul(m44 a, m44 b)
{
	m44 out;
	for(u32 i = 0; i < 4; i++)
	{
		for(u32 j = 0; j < 4; j++)
		{
			out.m[i][j] = 0.f;
			for(u32 k = 0; k < 4; k++)
			{
				out.m[i][j] += a.m[k][j] * b.m[i][k];
			};
		};
	};
	return out;
};

inline ray_t ray(v3 origin, v3 direction)
{
	ray_t ray;
	ray.origin = origin;
	ray.direction = direction;
	return ray;
};
inline v3 ray_point(ray_t ray, f32 t)
{
	return v3_add(ray.origin, v3_scale(ray.direction, t));
};

#endif