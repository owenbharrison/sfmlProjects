#include "particle.h"

#pragma once
struct constraint {
	particle* a = nullptr, * b = nullptr;
	float restLen = 0;

	constraint() {}

	constraint(particle& a_, particle& b_) : a(&a_), b(&b_) {
		restLen = length(a->pos - b->pos);
	}

	float2 getUpdateForce() {
		float2 axis = a->pos - b->pos;
		float dist = length(axis);
		float2 n = axis / dist;
		float delta = restLen - dist;
		return n * delta * .5f;
	}

	void update() {
		float2 force = getUpdateForce();
		if (!a->locked) a->pos += force;
		if (!b->locked) b->pos -= force;
	}
};