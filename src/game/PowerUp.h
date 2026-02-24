#pragma once
#include "core/Vec2.h"
#include "render/Color.h"
#include "render/IRenderer.h"

enum class PowerUpType {
	AutoAim,        // Turret auto-locks nearest enemy
	HomingRockets,  // N homing projectiles
	Shield,         // Absorb one hit
	RapidFire,      // Reduced fire cooldown
	Count
};

// Active power-up effect on a tank
struct ActivePowerUp {
	PowerUpType type = PowerUpType::AutoAim;
	float duration = 0.0f;  // remaining time (0 = expired, -1 = uses-based)
	int uses = 0;           // for HomingRockets

	bool IsExpired() const {
		if (uses > 0) return false;
		return duration <= 0.0f;
	}
};

// Power-up pickup in the arena
class PowerUp {
public:
	PowerUp(Vec2 position, PowerUpType type);

	void Update(float dt);
	void Render(IRenderer& renderer) const;

	Vec2 GetPosition() const { return m_position; }
	PowerUpType GetType() const { return m_type; }
	bool IsActive() const { return m_active; }
	float GetRadius() const { return RADIUS; }

	void Collect();
	void Respawn();

	// Per-type configuration
	static float GetDuration(PowerUpType type);
	static float GetRespawnDelay(PowerUpType type);
	static int GetUses(PowerUpType type);
	static Color GetColor(PowerUpType type);
	static const char* GetName(PowerUpType type);

	static constexpr float RADIUS = 12.0f;
	static constexpr float DEFAULT_RESPAWN_DELAY = 10.0f;

private:
	void RenderIcon(IRenderer& renderer) const;

	const Vec2 m_position;
	const PowerUpType m_type;
	bool m_active = true;
	float m_respawnTimer = 0.0f;
	float m_bobTimer = 0.0f; // for visual animation
};
