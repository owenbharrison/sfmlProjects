#include "../vector/float2.h"

#pragma once
struct aabb {
	float2 min, max;

	aabb() {}

	aabb(float nx, float ny, float mx, float my) : min(nx, ny), max(mx, my) {}

	aabb(float2 min, float2 max) : min(min), max(max) {}

	bool checkPt(float2 pt) {
		bool xOverlap=pt.x>=min.x&&pt.x<=max.x;
		bool yOverlap=pt.y>=min.y&&pt.y<=max.y;
		return xOverlap&&yOverlap;
	}

	bool checkAABB(aabb a) {
		bool xOverlap=min.x<=a.max.x&&max.x>=a.min.x;
		bool yOverlap=min.y<=a.max.y&&max.y>=a.min.y;
		return xOverlap&&yOverlap;
	}
};