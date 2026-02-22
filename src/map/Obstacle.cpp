#include "map/Obstacle.h"
#include "game/Collision.h"

Obstacle::Obstacle(const std::vector<Vec2>& vertices, Color color)
	: m_vertices(vertices)
	, m_color(color)
{
}

void Obstacle::Render(IRenderer& renderer) const {
	if (m_vertices.size() < 2) return;
	renderer.DrawPolyline(m_vertices, m_color, true);
}

bool Obstacle::ContainsPoint(Vec2 p) const {
	return Collision::PointInPolygon(p, m_vertices);
}
