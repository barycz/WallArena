#include "game/PowerUp.h"
#include <cmath>

static constexpr float PI = 3.14159265f;

PowerUp::PowerUp(Vec2 position, PowerUpType type)
	: m_position(position)
	, m_type(type)
	, m_active(true)
{
}

void PowerUp::Update(float dt) {
	m_bobTimer += dt;

	if (!m_active) {
		m_respawnTimer -= dt;
		if (m_respawnTimer <= 0.0f) {
			Respawn();
		}
	}
}

void PowerUp::Render(IRenderer& renderer) const {
	if (!m_active) return;

	Color c = GetColor(m_type);

	// Pulsing outer circle
	float pulse = 1.0f + 0.15f * std::sin(m_bobTimer * 4.0f);
	renderer.DrawCircle(m_position, RADIUS * pulse, c, 8);

	// Inner icon per type
	RenderIcon(renderer);
}

void PowerUp::RenderIcon(IRenderer& renderer) const {
	Color c = GetColor(m_type);
	float r = RADIUS * 0.5f;

	switch (m_type) {
		case PowerUpType::AutoAim: {
			// Crosshair icon
			renderer.DrawLine(m_position + Vec2(-r, 0), m_position + Vec2(r, 0), c);
			renderer.DrawLine(m_position + Vec2(0, -r), m_position + Vec2(0, r), c);
			renderer.DrawCircle(m_position, r * 0.5f, c, 6);
			break;
		}
		case PowerUpType::HomingRockets: {
			// Arrow/rocket pointing up
			Vec2 top = m_position + Vec2(0, -r);
			Vec2 bl = m_position + Vec2(-r * 0.5f, r);
			Vec2 br = m_position + Vec2(r * 0.5f, r);
			renderer.DrawLine(top, bl, c);
			renderer.DrawLine(top, br, c);
			renderer.DrawLine(bl, br, c);
			break;
		}
		case PowerUpType::Shield: {
			// Shield arc
			std::vector<Vec2> arc;
			for (int i = 0; i <= 6; ++i) {
				float angle = PI * 0.2f + (PI * 0.6f) * i / 6.0f;
				arc.push_back(m_position + Vec2(std::cos(angle), -std::sin(angle)) * r);
			}
			renderer.DrawPolyline(arc, c, false);
			break;
		}
		case PowerUpType::RapidFire: {
			// Multiple small lines (bullets)
			for (int i = -1; i <= 1; ++i) {
				float ox = i * r * 0.5f;
				renderer.DrawLine(m_position + Vec2(ox, -r * 0.3f),
								  m_position + Vec2(ox, r * 0.3f), c);
			}
			break;
		}
		default:
			break;
	}
}

void PowerUp::Collect() {
	m_active = false;
	m_respawnTimer = m_respawnDelay;
}

void PowerUp::Respawn() {
	m_active = true;
	m_respawnTimer = 0.0f;
}

float PowerUp::GetDuration(PowerUpType type) {
	switch (type) {
		case PowerUpType::AutoAim:       return 8.0f;
		case PowerUpType::HomingRockets: return -1.0f; // uses-based
		case PowerUpType::Shield:        return 15.0f;
		case PowerUpType::RapidFire:     return 8.0f;
		default: return 5.0f;
	}
}

int PowerUp::GetUses(PowerUpType type) {
	switch (type) {
		case PowerUpType::HomingRockets: return 3;
		default: return 0;
	}
}

Color PowerUp::GetColor(PowerUpType type) {
	switch (type) {
		case PowerUpType::AutoAim:       return Color::Cyan();
		case PowerUpType::HomingRockets: return Color(255, 128, 0); // orange
		case PowerUpType::Shield:        return Color(100, 200, 255);
		case PowerUpType::RapidFire:     return Color::Yellow();
		default: return Color::White();
	}
}

const char* PowerUp::GetName(PowerUpType type) {
	switch (type) {
		case PowerUpType::AutoAim:       return "AUTO-AIM";
		case PowerUpType::HomingRockets: return "HOMING";
		case PowerUpType::Shield:        return "SHIELD";
		case PowerUpType::RapidFire:     return "RAPID FIRE";
		default: return "???";
	}
}
