#pragma once
#include <vector>
#include "core/Vec2.h"
#include "render/Color.h"

class IRenderer {
public:
	virtual ~IRenderer() = default;

	virtual bool Init(int windowWidth, int windowHeight) = 0;
	virtual void Shutdown() = 0;

	virtual void Begin() = 0;
	virtual void End() = 0;

	virtual void DrawLine(Vec2 a, Vec2 b, Color c) = 0;
	virtual void DrawPolyline(const std::vector<Vec2>& pts, Color c, bool closed = false) = 0;
	virtual void DrawCircle(Vec2 center, float radius, Color c, int segments = 16) = 0;
	virtual void DrawRect(Vec2 topLeft, Vec2 size, Color c) = 0;

	virtual void SetWorldBounds(float width, float height) = 0;
};
