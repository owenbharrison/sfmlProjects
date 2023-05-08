#include "../vector/float2.h"

#pragma once
struct fx {
	float2 pos, vel, acc;
	float rad = 0, lifeSpan = 0, age = 0;

	fx() {}

	fx(float2 pos_, float2 vel_, float rad_, float lifeSpan_) : pos(pos_), vel(vel_), rad(rad_), lifeSpan(lifeSpan_) {}

	void update(float dt) {
		vel += acc * dt;
		pos += vel * dt;
		acc *= 0;

		age += dt;
	}

	void accelerate(float2 f) { acc += f; }

	bool isDead() {
		return age > lifeSpan;
	}
};
