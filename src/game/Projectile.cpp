#include "game/Projectile.h"

Projectile::Projectile(Vec2 position, Vec2 velocity, int ownerIndex, Color color)
	: m_position(position)
	, m_prevPosition(position)
	, m_velocity(velocity)
	, m_color(color)
	, m_ownerIndex(ownerIndex)
	, m_alive(true)
	, m_lifetime(LIFETIME)
{
}

void Projectile::Update(float dt) {
	if (!m_alive) return;

	m_prevPosition = m_position;
	m_position += m_velocity * dt;

	m_lifetime -= dt;
	if (m_lifetime <= 0.0f) {
		m_alive = false;
	}
}

void Projectile::Render(IRenderer& renderer) const {
	if (!m_alive) return;

	Vec2 dir = m_velocity.Normalized();
	Vec2 tail = m_position - dir * LENGTH;
	renderer.DrawLine(tail, m_position, m_color);
	renderer.DrawCircle(m_position, 2.0f, m_color, 5);
}
