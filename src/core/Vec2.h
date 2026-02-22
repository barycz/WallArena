#pragma once
#include <cmath>

struct Vec2 {
	float x = 0.0f;
	float y = 0.0f;

	Vec2() = default;
	Vec2(float x, float y) : x(x), y(y) {}

	Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
	Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
	Vec2 operator*(float s) const { return {x * s, y * s}; }
	Vec2 operator/(float s) const { return {x / s, y / s}; }

	Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
	Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
	Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

	float Dot(const Vec2& o) const { return x * o.x + y * o.y; }
	float Cross(const Vec2& o) const { return x * o.y - y * o.x; }
	float Length() const { return std::sqrt(x * x + y * y); }
	float LengthSq() const { return x * x + y * y; }

	Vec2 Normalized() const {
		float len = Length();
		if (len < 1e-6f) return {0.0f, 0.0f};
		return {x / len, y / len};
	}

	Vec2 Rotated(float radians) const {
		float c = std::cos(radians);
		float s = std::sin(radians);
		return {x * c - y * s, x * s + y * c};
	}

	float Angle() const { return std::atan2(y, x); }

	static Vec2 FromAngle(float radians) {
		return {std::cos(radians), std::sin(radians)};
	}

	static float Distance(const Vec2& a, const Vec2& b) {
		return (a - b).Length();
	}
};

inline Vec2 operator*(float s, const Vec2& v) { return {v.x * s, v.y * s}; }
