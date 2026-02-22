#pragma once
#include "render/IRenderer.h"
#include "core/Vec2.h"
#include "render/Color.h"
#include <string>

// Simple vector font: each character is defined as a series of line segments
// on a 5x7 grid. Renders text using IRenderer::DrawLine calls.
class VectorFont {
public:
	static void DrawText(IRenderer& renderer, const std::string& text,
						 Vec2 position, float scale, Color color);

	static float MeasureWidth(const std::string& text, float scale);

	// Draw text centered horizontally at the given Y position within a width
	static void DrawTextCentered(IRenderer& renderer, const std::string& text,
								 float centerX, float y, float scale, Color color);

	struct Segment { float x1, y1, x2, y2; };

private:
	static const Segment* GetCharSegments(char c, int& count);

	static constexpr float CHAR_WIDTH = 5.0f;
	static constexpr float CHAR_HEIGHT = 7.0f;
	static constexpr float CHAR_SPACING = 1.0f;
};
