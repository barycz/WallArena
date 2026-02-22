#pragma once
#include "core/Vec2.h"
#include "render/Color.h"
#include "render/IRenderer.h"

class Projectile {
public:
	Projectile() = default;
	Projectile(Vec2 position, Vec2 velocity, int ownerIndex, Color color);

	void Update(float dt);
	void Render(IRenderer& renderer) const;

	Vec2 GetPosition() const { return m_position; }
	Vec2 GetPrevPosition() const { return m_prevPosition; }
	Vec2 GetVelocity() const { return m_velocity; }
	int GetOwnerIndex() const { return m_ownerIndex; }
	bool IsAlive() const { return m_alive; }
	Color GetColor() const { return m_color; }

	void Kill() { m_alive = false; }
	void SetVelocity(Vec2 vel) { m_velocity = vel; }
	void SetPosition(Vec2 pos) { m_position = pos; }

	static constexpr float SPEED = 500.0f;
	static constexpr float LIFETIME = 3.0f;
	static constexpr float LENGTH = 8.0f;

private:
	Vec2 m_position;
	Vec2 m_prevPosition;
	Vec2 m_velocity;
	Color m_color;
	int m_ownerIndex = -1;
	bool m_alive = true;
	float m_lifetime = LIFETIME;
};
