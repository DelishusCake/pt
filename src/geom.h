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
	f32 v[3];
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
	v3 min;
	v3 max;
} aabb_t;

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
inline v3 v3_lerp(v3 a, v3 b, f32 t)
{
	return v3_add(v3_scale(a, t), v3_scale(b, (1.f - t)));
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
inline v4 m44_transform(m44 m, v4 v)
{
	v4 r;
	r.x = m.x0*v.x + m.x1*v.y + m.x2*v.z + m.x3*v.w;
	r.y = m.y0*v.x + m.y1*v.y + m.y2*v.z + m.y3*v.w;
	r.z = m.z0*v.x + m.z1*v.y + m.z2*v.z + m.z3*v.w;
	r.w = m.w0*v.x + m.w1*v.y + m.w2*v.z + m.w3*v.w;
	return r;
};
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
inline m44 m44_lookAt(v3 eye, v3 at, v3 up)
{
	const v3 f = v3_norm(v3_sub(at, eye));
	const v3 s = v3_norm(v3_cross(f, up));
	const v3 u = v3_cross(s, f);

	const f32 tx = -v3_dot(s, eye);
	const f32 ty = -v3_dot(u, eye);
	const f32 tz =  v3_dot(f, eye);

	return (m44)
	{{
		s.x, u.x, -f.x, 0.f,
		s.y, u.y, -f.y, 0.f,
		s.z, u.z, -f.z, 0.f,
		tx,  ty,   tz,  1.f,
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
inline m44 m44_perspective(f32 y_fov, f32 aspect, f32 zn, f32 zf)
{
	const f32 a = 1.f / f32_tan(y_fov / 2.f);

	const f32 sx = a / aspect;
	const f32 sy = a;
	const f32 sz = -((zf + zn) / (zf-zn));

	const f32 tz = -((2.f*zf*zn) / (zf-zn));

	return (m44)
	{{
		sx, 0.f, 0.f,  0.f,
		0.f, sy, 0.f,  0.f,
		0.f, 0.f, sz, -1.f,
		0.f, 0.f, tz,  0.f,
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
static inline m44 m44_viewport(i32 x, i32 y, i32 w, i32 h)
{
	const i32 d = 255;
	return (m44)
	{{
		w/2, 0, 0, 0,
		0, h/2, 0, 0,
		0, 0, d/2, 0,
		x+(w/2), y+(h/2), d/2, 1,
	}};
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

inline bool aabb_hit(aabb_t aabb, ray_t ray, f32 t_min, f32 t_max)
{
	for (u32 i = 0; i < 3; i++)
	{
		f32 i_d = 1.f / ray.direction.v[i];
		f32 t_0 = (aabb.min.v[i] - ray.origin.v[i]) * i_d;
		f32 t_1 = (aabb.max.v[i] - ray.origin.v[i]) * i_d;
		if (i_d < 0.f)
			swap(f32, t_0, t_1);
		t_min = t_0 > t_min ? t_0 : t_min;
		t_max = t_1 < t_max ? t_1 : t_max;
		if (t_max <= t_min)
			return false;
	}
	return true;
};
inline aabb_t aabb_combine(aabb_t a, aabb_t b)
{
	aabb_t aabb;
	aabb.min = V3(
		min(a.min.x, b.min.x),
		min(a.min.y, b.min.y),
		min(a.min.z, b.min.z));
	aabb.max = V3(
		max(a.max.x, b.max.x),
		max(a.max.y, b.max.y),
		max(a.max.z, b.max.z));
	return aabb;
};

#endif