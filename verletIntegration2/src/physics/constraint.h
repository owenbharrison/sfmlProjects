#include "particle.h"

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#pragma once
struct constraint {
	particle* a=nullptr, * b=nullptr;
	float rad=0, restLen=0;

	constraint() {}

	constraint(particle& a_, particle& b_, float rad) : a(&a_), b(&b_), rad(rad) {
		restLen=length(a->pos-b->pos);
	}

	float2 getForce() {
		float2 axis=a->pos-b->pos;
		float dist=length(axis);
		float2 n=axis/dist;
		float delta=restLen-dist;
		return n*delta*.5f;
	}

	void update() {
		float2 f=getForce();
		float totalMass=a->mass+b->mass;
		if (!a->locked) a->pos+=b->mass/totalMass*2*f;
		if (!b->locked) b->pos-=a->mass/totalMass*2*f;
	}

	aabb getAABB() {
		return aabb(
			MIN(a->pos.x, b->pos.x)-rad,
			MIN(a->pos.y, b->pos.y)-rad,
			MAX(a->pos.x, b->pos.x)+rad,
			MAX(a->pos.y, b->pos.y)+rad
		);
	}
};