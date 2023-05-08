#include "aabb.h"

#define PI 3.1415927

#pragma once
struct particle {
	float2 pos, oldpos, force;
	float rad=0, mass=0;
	bool locked=false;

	particle() {}

	particle(float2 pos, float rad) : pos(pos), oldpos(pos), rad(rad) {
		mass=PI*rad*rad;
	}

	void update(float dt) {
		float2 vel=pos-oldpos;
		//save pos
		oldpos=pos;
		//verlet integrate
		if (!locked) pos+=vel+force/mass*dt*dt;
		//reset forces
		force*=0;
	}

	void applyForce(float2 f) {
		if (!locked) force+=f;
	}

	aabb getAABB() const {
		return aabb(pos-rad, pos+rad);
	}

	void checkAABB(aabb a) {
		float2 vel=pos-oldpos;
		if (pos.x<a.min.x+rad) pos.x=a.min.x+rad, oldpos.x=pos.x+vel.x;
		if (pos.y<a.min.y+rad) pos.y=a.min.y+rad, oldpos.y=pos.y+vel.y;
		if (pos.x>a.max.x-rad) pos.x=a.max.x-rad, oldpos.x=pos.x+vel.x;
		if (pos.y>a.max.y-rad) pos.y=a.max.y-rad, oldpos.y=pos.y+vel.y;
	}
};