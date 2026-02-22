#pragma once
#include "core/Vec2.h"
#include <vector>

namespace Collision {

// Test if two line segments (a1-a2) and (b1-b2) intersect.
// If they do, returns true and sets 'point' to the intersection point.
bool SegmentSegment(Vec2 a1, Vec2 a2, Vec2 b1, Vec2 b2, Vec2& point);

// Test if a point is inside a polygon (convex or concave, vertices in order).
bool PointInPolygon(Vec2 point, const std::vector<Vec2>& polygon);

// Test if a moving point (from prev to curr) hits any edge of a convex polygon.
// Returns true if hit, sets hitPoint and hitNormal.
bool SegmentPolygon(Vec2 prev, Vec2 curr, const std::vector<Vec2>& polygon,
					Vec2& hitPoint, Vec2& hitNormal);

// Clamp a position to stay within an axis-aligned rectangle.
Vec2 ClampToRect(Vec2 pos, float minX, float minY, float maxX, float maxY);

// Push a convex polygon out of an AABB boundary (returns displacement needed).
Vec2 PushOutOfBounds(const std::vector<Vec2>& polygon,
					 float minX, float minY, float maxX, float maxY);

} // namespace Collision
