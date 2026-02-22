#pragma once
#include "core/Vec2.h"
#include "render/Color.h"
#include "render/IRenderer.h"
#include <vector>
#include <string>

class Obstacle {
public:
	Obstacle() = default;
	Obstacle(const std::vector<Vec2>& vertices, Color color = Color::White());

	void Render(IRenderer& renderer) const;

	const std::vector<Vec2>& GetVertices() const { return m_vertices; }
	void SetVertices(const std::vector<Vec2>& verts) { m_vertices = verts; }
	void AddVertex(Vec2 v) { m_vertices.push_back(v); }
	void RemoveLastVertex() { if (!m_vertices.empty()) m_vertices.pop_back(); }

	Color GetColor() const { return m_color; }
	void SetColor(Color c) { m_color = c; }

	// Wall-feature tag for projector alignment
	const std::string& GetTag() const { return m_tag; }
	void SetTag(const std::string& tag) { m_tag = tag; }

	bool IsDestructible() const { return m_destructible; }
	void SetDestructible(bool d) { m_destructible = d; }

	// Bounding box test
	bool ContainsPoint(Vec2 p) const;

	// Get edge count
	int GetEdgeCount() const { return static_cast<int>(m_vertices.size()); }

private:
	std::vector<Vec2> m_vertices;
	Color m_color = Color::White();
	std::string m_tag;
	bool m_destructible = false;
};
