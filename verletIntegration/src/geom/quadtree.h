#include "aabb.h"
#include <vector>

template<class T>
void addAll(std::vector<T>& a, const std::vector<T>& b) {
	a.insert(a.end(), b.begin(), b.end());
}

struct quadTree {
	const int cap = 1;
	aabb bounds;
	std::vector<float2> points;

	quadTree* northWest = nullptr;
	quadTree* northEast = nullptr;
	quadTree* southWest = nullptr;
	quadTree* southEast = nullptr;

	quadTree(aabb bounds_) {
		bounds = bounds_;
	}

	~quadTree(){
		if(northWest!=nullptr){
			delete northWest;
			delete northEast;
			delete southWest;
			delete southEast;
		}
	}

	void subdivide() {
		float2 ctr = (bounds.min + bounds.max) / 2;
		northWest = new quadTree(aabb(bounds.min, ctr));
		northEast = new quadTree(aabb(float2(ctr.x, bounds.min.y), float2(bounds.max.x, ctr.y)));
		southWest = new quadTree(aabb(float2(bounds.min.x, ctr.y), float2(ctr.x, bounds.max.y)));
		southEast = new quadTree(aabb(ctr, bounds.max));
	}

	bool insert(float2 pt) {
		if (!bounds.containsPt(pt))
			return false;

		if (northWest == nullptr) {
			if (points.size() < cap) {
				points.push_back(pt);
				return true;
			}
			subdivide();
		}

		if (northWest->insert(pt)) return true;
		if (northEast->insert(pt)) return true;
		if (southWest->insert(pt)) return true;
		if (southEast->insert(pt)) return true;

		return false;
	}

	std::vector<float2> queryRange(aabb range) {
		std::vector<float2> pts;
		if (!bounds.overlapAABB(range))
			return pts; // empty list

		for (float2& pt : points) {
			if (range.containsPt(pt)) pts.push_back(pt);
		}

		if (northWest == nullptr) return pts;

		addAll(pts, northWest->queryRange(range));
		addAll(pts, northEast->queryRange(range));
		addAll(pts, southWest->queryRange(range));
		addAll(pts, southEast->queryRange(range));

		return pts;
	}
};