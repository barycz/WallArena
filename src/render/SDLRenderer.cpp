#include "render/SDLRenderer.h"
#include <cmath>

SDLRenderer::SDLRenderer() = default;

SDLRenderer::~SDLRenderer() {
	Shutdown();
}

bool SDLRenderer::Init(int windowWidth, int windowHeight) {
	m_windowWidth = windowWidth;
	m_windowHeight = windowHeight;

	m_window = SDL_CreateWindow(
		"Wall Arena",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		windowWidth, windowHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);
	if (!m_window) return false;

	m_renderer = SDL_CreateRenderer(m_window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!m_renderer) {
		SDL_DestroyWindow(m_window);
		m_window = nullptr;
		return false;
	}

	return true;
}

void SDLRenderer::Shutdown() {
	if (m_renderer) { SDL_DestroyRenderer(m_renderer); m_renderer = nullptr; }
	if (m_window)   { SDL_DestroyWindow(m_window);     m_window = nullptr; }
}

void SDLRenderer::Begin() {
	SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
	SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
	SDL_RenderClear(m_renderer);
}

void SDLRenderer::End() {
	SDL_RenderPresent(m_renderer);
}

void SDLRenderer::WorldToScreen(Vec2 world, int& sx, int& sy) const {
	float scaleX = static_cast<float>(m_windowWidth) / m_worldWidth;
	float scaleY = static_cast<float>(m_windowHeight) / m_worldHeight;
	float scale = std::min(scaleX, scaleY);

	float offsetX = (m_windowWidth - m_worldWidth * scale) * 0.5f;
	float offsetY = (m_windowHeight - m_worldHeight * scale) * 0.5f;

	sx = static_cast<int>(world.x * scale + offsetX);
	sy = static_cast<int>(world.y * scale + offsetY);
}

void SDLRenderer::DrawLine(Vec2 a, Vec2 b, Color c) {
	int x1, y1, x2, y2;
	WorldToScreen(a, x1, y1);
	WorldToScreen(b, x2, y2);
	SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
	SDL_RenderDrawLine(m_renderer, x1, y1, x2, y2);
}

void SDLRenderer::DrawPolyline(const std::vector<Vec2>& pts, Color c, bool closed) {
	if (pts.size() < 2) return;
	for (size_t i = 0; i + 1 < pts.size(); ++i) {
		DrawLine(pts[i], pts[i + 1], c);
	}
	if (closed && pts.size() > 2) {
		DrawLine(pts.back(), pts.front(), c);
	}
}

void SDLRenderer::DrawCircle(Vec2 center, float radius, Color c, int segments) {
	std::vector<Vec2> pts(segments);
	for (int i = 0; i < segments; ++i) {
		float angle = (2.0f * 3.14159265f * i) / segments;
		pts[i] = center + Vec2::FromAngle(angle) * radius;
	}
	DrawPolyline(pts, c, true);
}

void SDLRenderer::DrawRect(Vec2 topLeft, Vec2 size, Color c) {
	Vec2 a = topLeft;
	Vec2 b = {topLeft.x + size.x, topLeft.y};
	Vec2 d = {topLeft.x, topLeft.y + size.y};
	Vec2 e = {topLeft.x + size.x, topLeft.y + size.y};
	DrawLine(a, b, c);
	DrawLine(b, e, c);
	DrawLine(e, d, c);
	DrawLine(d, a, c);
}

void SDLRenderer::SetWorldBounds(float width, float height) {
	m_worldWidth = width;
	m_worldHeight = height;
}
