#include "../geom/aabb.h"

#define PI 3.1415926

//thanks to pezzza for verlet stuff!
//https://www.youtube.com/watch?v=lS_qeBy3aQI&t=181s&ab_channel=Pezzza%27sWork
#pragma once
struct particle {
	float2 pos, oldpos, acc;
	float rad=0;
	bool locked=false;
	int id=-1;

	particle() {}

	particle(float2 pos_, float rad_) : pos(pos_), oldpos(pos_), rad(rad_) {}

	void update(float dt) {
		float2 vel=pos-oldpos;
		//save pos
		oldpos=pos;
		//verlet integrate
		if(!locked) pos+=vel+acc*dt*dt;
		//reset acc
		acc*=0;
	}

	void accelerate(float2 f) {
		if(!locked) acc+=f;
	}

	aabb getAABB() const {
		return aabb(pos-rad, pos+rad);
	}

	void checkAABB(aabb a) {
		float2 vel=pos-oldpos;

		if (pos.x<a.min.x+rad) pos.x=a.min.x+rad;
		if (pos.y<a.min.y+rad) pos.y=a.min.y+rad;
		if (pos.x>a.max.x-rad) pos.x=a.max.x-rad;
		if (pos.y>a.max.y-rad) pos.y=a.max.y-rad;
	}
};