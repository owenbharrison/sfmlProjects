#pragma once

#include <cmath>

namespace displib {
	struct vec3d {
		float x, y, z, w=1.f;

		vec3d() : vec3d(0.f) {}

		vec3d(float f) : vec3d(f, f, f) {}

		vec3d(float x, float y, float z) : x(x), y(y), z(z) {}

		vec3d(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

		vec3d operator+(vec3d v) { return {x+v.x, y+v.y, z+v.z}; }
		vec3d operator+(float f) { return operator+({f, f, f}); }
		void operator+=(vec3d v) { *this=*this+v; }
		void operator+=(float f) { *this=*this+f; }

		vec3d operator-(vec3d v) { return {x-v.x, y-v.y, z-v.z}; }
		vec3d operator-(float f) { return operator-({f, f, f}); }
		void operator-=(vec3d v) { *this=*this-v; }
		void operator-=(float f) { *this=*this-f; }

		vec3d operator*(vec3d v) { return {x*v.x, y*v.y, z*v.z}; }
		vec3d operator*(float f) { return operator*({f, f, f}); }
		void operator*=(vec3d v) { *this=*this*v; }
		void operator*=(float f) { *this=*this*f; }

		vec3d operator/(vec3d v) { return {x/v.x, y/v.y, z/v.z}; }
		vec3d operator/(float f) { return operator/({f, f, f}); }
		void operator/=(vec3d v) { *this=*this/v; }
		void operator/=(float f) { *this=*this/f; }
	};

	vec3d operator+(float f, vec3d v) { return v+f; }
	vec3d operator-(float f, vec3d v) { return v-f; }
	vec3d operator*(float f, vec3d v) { return v*f; }
	vec3d operator/(float f, vec3d v) { return v/f; }

	float dot(vec3d a, vec3d b) {
		vec3d c=a*b;
		return c.x+c.y+c.z;
	}

	float length(vec3d a) {
		return sqrtf(dot(a, a));
	}

	vec3d normalize(vec3d a) {
		float l=length(a);
		return l!=0.f?a/l:a;
	}

	vec3d cross(vec3d a, vec3d b) {
		return {
			a.y*b.z-a.z*b.y,
			a.z*b.x-a.x*b.z,
			a.x*b.y-a.y*b.x
		};
	}
}