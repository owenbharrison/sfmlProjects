#include "../vector/float3.h"

#pragma once
struct aabb {
	float3 min, max;

	aabb() {};

	aabb(float nx, float ny, float nz, float mx, float my, float mz) : min(nx, ny, nz), max(mx, my, mz) {}

	aabb(float3 min_, float3 max_) : min(min_), max(max_) {}

	bool containsPt(float3 pt) {
		bool xOverlap = pt.x >= min.x && pt.x <= max.x;
		bool yOverlap = pt.y >= min.y && pt.y <= max.y;
		bool zOverlap = pt.z >= min.z && pt.z <= max.z;
		return xOverlap && yOverlap && zOverlap;
	}

	bool overlapAABB(aabb a) {
		bool xOverlap = min.x <= a.max.x && max.x >= a.min.x;
		bool yOverlap = min.y <= a.max.y && max.y >= a.min.y;
		bool zOverlap = min.z <= a.max.z && max.z >= a.min.z;
		return xOverlap && yOverlap && zOverlap;
	}
};