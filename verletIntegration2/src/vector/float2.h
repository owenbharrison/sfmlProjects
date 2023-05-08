#include <cmath>

#pragma once
struct float2 {
	float x, y;

	float2() : float2(0) {}

	float2(float f) : x(f), y(f) {}

	float2(float x, float y) : x(x), y(y) {}

	//check
	bool operator==(float2 f) {
		return x==f.x&&y==f.y;
	}

	//negate
	float2 operator-() { return *this*-1; }

	//vec by vec
	float2 operator+(float2 a) const { return float2(x+a.x, y+a.y); }
	float2 operator-(float2 a) const { return float2(x-a.x, y-a.y); }
	float2 operator*(float2 a) const { return float2(x*a.x, y*a.y); }
	float2 operator/(float2 a) const { return float2(x/a.x, y/a.y); }
	void operator+=(float2 a) { *this=*this+a; }
	void operator-=(float2 a) { *this=*this-a; }
	void operator*=(float2 a) { *this=*this*a; }
	void operator/=(float2 a) { *this=*this/a; }

	//vec by scalar
	float2 operator+(float a) const { return *this+float2(a); }
	float2 operator-(float a) const { return *this-float2(a); }
	float2 operator*(float a) const { return *this*float2(a); }
	float2 operator/(float a) const { return *this/float2(a); }
	void operator+=(float a) { *this=*this+a; }
	void operator-=(float a) { *this=*this-a; }
	void operator*=(float a) { *this=*this*a; }
	void operator/=(float a) { *this=*this/a; }
};

//scalar by vec
float2 operator+(float a, float2 b) { return b+a; }
float2 operator-(float a, float2 b) { return b-a; }
float2 operator*(float a, float2 b) { return b*a; }
float2 operator/(float a, float2 b) { return b/a; }

float dot(float2 a, float2 b) {
	float2 c=a*b;
	return c.x+c.y;
}

float length(float2 a) {
	return sqrtf(dot(a, a));
}

float2 normalize(float2 a) {
	float l=length(a);
	return l==0?a:a/l;
}