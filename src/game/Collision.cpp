#include "game/Collision.h"
#include <algorithm>
#include <cmath>

namespace Collision {

bool SegmentSegment(Vec2 a1, Vec2 a2, Vec2 b1, Vec2 b2, Vec2& point) {
	Vec2 d1 = a2 - a1;
	Vec2 d2 = b2 - b1;
	float cross = d1.Cross(d2);

	if (std::abs(cross) < 1e-8f) return false; // parallel

	Vec2 d = b1 - a1;
	float t = d.Cross(d2) / cross;
	float u = d.Cross(d1) / cross;

	if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
		point = a1 + d1 * t;
		return true;
	}
	return false;
}

bool PointInPolygon(Vec2 point, const std::vector<Vec2>& polygon) {
	int n = static_cast<int>(polygon.size());
	if (n < 3) return false;

	// Ray-casting algorithm: works for convex and concave polygons
	bool inside = false;
	for (int i = 0, j = n - 1; i < n; j = i++) {
		float yi = polygon[i].y, yj = polygon[j].y;
		float xi = polygon[i].x, xj = polygon[j].x;
		if ((yi > point.y) != (yj > point.y) &&
			point.x < (xj - xi) * (point.y - yi) / (yj - yi) + xi) {
			inside = !inside;
		}
	}
	return inside;
}

bool SegmentPolygon(Vec2 prev, Vec2 curr, const std::vector<Vec2>& polygon,
					Vec2& hitPoint, Vec2& hitNormal) {
	int n = static_cast<int>(polygon.size());
	float closestT = 2.0f;
	bool hit = false;

	for (int i = 0; i < n; ++i) {
		Vec2 e1 = polygon[i];
		Vec2 e2 = polygon[(i + 1) % n];

		Vec2 d1 = curr - prev;
		Vec2 d2 = e2 - e1;
		float cross = d1.Cross(d2);
		if (std::abs(cross) < 1e-8f) continue;

		Vec2 d = e1 - prev;
		float t = d.Cross(d2) / cross;
		float u = d.Cross(d1) / cross;

		if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
			if (t < closestT) {
				closestT = t;
				hitPoint = prev + d1 * t;
				// Normal is perpendicular to edge, pointing outward
				Vec2 edge = e2 - e1;
				hitNormal = Vec2(-edge.y, edge.x).Normalized();
				hit = true;
			}
		}
	}
	return hit;
}

Vec2 ClampToRect(Vec2 pos, float minX, float minY, float maxX, float maxY) {
	return {
		std::max(minX, std::min(maxX, pos.x)),
		std::max(minY, std::min(maxY, pos.y))
	};
}

Vec2 PushOutOfBounds(const std::vector<Vec2>& polygon,
					 float minX, float minY, float maxX, float maxY) {
	Vec2 push = {0.0f, 0.0f};
	for (const auto& p : polygon) {
		if (p.x < minX) push.x = std::max(push.x, minX - p.x);
		if (p.x > maxX) push.x = std::min(push.x, maxX - p.x);
		if (p.y < minY) push.y = std::max(push.y, minY - p.y);
		if (p.y > maxY) push.y = std::min(push.y, maxY - p.y);
	}
	return push;
}

} // namespace Collision
