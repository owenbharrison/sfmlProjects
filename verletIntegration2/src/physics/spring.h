#include "particle.h"

#pragma once
struct spring {
	particle* a=nullptr, * b=nullptr;
	float restLen=0, stiffness=0, damping=0;

	spring() {}

	spring(particle& a_, particle& b_, float stiffness, float damping) : a(&a_), b(&b_), stiffness(stiffness), damping(damping) {
		restLen=length(a->pos-b->pos);
	}

	float2 getForce(float dt){
		float2 sub=b->pos-a->pos;
		float currLen=length(sub);
		float springForce=stiffness*(currLen-restLen);
		float2 norm=sub/currLen;
		float2 aVel=(a->pos-a->oldpos)/dt;
		float2 bVel=(b->pos-b->oldpos)/dt;
		float dampForce=damping*dot(norm, bVel-aVel);
		return norm*(springForce+dampForce);
	}

	void update(float dt) {
		float2 force=getForce(dt);
		if(!a->locked) a->applyForce(force);
		if(!b->locked) b->applyForce(-force);
	}
};