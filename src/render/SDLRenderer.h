#pragma once
#include "render/IRenderer.h"
#include <SDL.h>

class SDLRenderer : public IRenderer {
public:
	SDLRenderer();
	~SDLRenderer() override;

	bool Init(int windowWidth, int windowHeight) override;
	void Shutdown() override;

	void Begin() override;
	void End() override;

	void DrawLine(Vec2 a, Vec2 b, Color c) override;
	void DrawPolyline(const std::vector<Vec2>& pts, Color c, bool closed = false) override;
	void DrawCircle(Vec2 center, float radius, Color c, int segments = 16) override;
	void DrawRect(Vec2 topLeft, Vec2 size, Color c) override;

	void SetWorldBounds(float width, float height) override;

	SDL_Window* GetWindow() const { return m_window; }

private:
	void WorldToScreen(Vec2 world, int& sx, int& sy) const;

	SDL_Window* m_window = nullptr;
	SDL_Renderer* m_renderer = nullptr;
	int m_windowWidth = 0;
	int m_windowHeight = 0;
	float m_worldWidth = 1000.0f;
	float m_worldHeight = 1000.0f;
};
