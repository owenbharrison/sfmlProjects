#include "particle.h"

#pragma once
struct constraint {
	particle* a = nullptr, * b = nullptr;
	float restLen = 0;

	constraint() {}

	constraint(particle& a_, particle& b_) : a(&a_), b(&b_) {
		restLen = length(a->pos - b->pos);
	}

	void update() {
		float3 axis = a->pos - b->pos;
		float dist = length(axis);
		float3 n = axis / dist;
		float delta = restLen - dist;
		if (!a->locked) a->pos += n * delta * .5f;
		if (!b->locked) b->pos -= n * delta * .5f;
	}
};