#pragma once
#include "core/Vec2.h"
#include "render/Color.h"
#include "render/IRenderer.h"
#include "input/PlayerInput.h"
#include "game/PowerUp.h"
#include <vector>
#include <optional>

class Tank {
public:
	Tank();
	Tank(int playerIndex, Vec2 spawnPos, float spawnAngle, Color color);

	void Update(float dt, const PlayerInput& input);
	void Render(IRenderer& renderer) const;

	// Getters
	Vec2 GetPosition() const { return m_position; }
	float GetAngle() const { return m_angle; }
	Vec2 GetForward() const { return Vec2::FromAngle(m_angle); }
	float GetCannonAngle() const { return m_cannonAngle; }
	void SetCannonAngle(float angle) { m_cannonAngle = angle; }
	Vec2 GetCannonForward() const { return Vec2::FromAngle(m_cannonAngle); }
	Vec2 GetCannonTip() const;
	Color GetColor() const { return m_color; }
	int GetPlayerIndex() const { return m_playerIndex; }
	bool IsAlive() const { return m_alive; }
	int GetHealth() const { return m_health; }
	float GetRespawnTimer() const { return m_respawnTimer; }

	// Gameplay
	bool CanFire() const;
	void ResetFireCooldown();
	void TakeDamage(int amount);
	void Kill();
	void Respawn(Vec2 pos, float angle);
	void SetPosition(Vec2 pos) { m_position = pos; }

	// Power-up system
	void ApplyPowerUp(PowerUpType type);
	void UpdatePowerUp(float dt);
	bool HasPowerUp(PowerUpType type) const;
	bool HasShield() const;
	void ConsumeShield();
	bool HasAutoAim() const;
	float GetFireCooldownMultiplier() const;
	int GetHomingRocketUses() const;
	void ConsumeHomingRocket();
	std::optional<ActivePowerUp> GetActivePowerUp() const { return m_activePowerUp; }

	// Collision shape: returns the 4 corners of the tank body in world space
	std::vector<Vec2> GetBodyPolygon() const;

	// Stats
	int kills = 0;
	int deaths = 0;

	// Movement tuning
	static constexpr float MOVE_SPEED = 200.0f;
	static constexpr float TURN_SPEED = 4.0f;       // radians/sec
	static constexpr float BODY_HALF_W = 15.0f;
	static constexpr float BODY_HALF_H = 20.0f;
	static constexpr float COLLISION_RADIUS = 18.0f;
	static constexpr float CANNON_LENGTH = 25.0f;
	static constexpr float FIRE_COOLDOWN = 0.35f;
	static constexpr int   MAX_HEALTH = 3;
	static constexpr float RESPAWN_DELAY = 1.5f;
	static constexpr float INVULN_TIME = 2.0f;

private:
	int m_playerIndex = 0;
	Vec2 m_position;
	float m_angle = 0.0f;       // body angle, radians, 0 = right
	float m_cannonAngle = 0.0f;  // cannon angle, independent when auto-aim active
	Color m_color;

	bool m_alive = true;
	int m_health = MAX_HEALTH;
	float m_fireCooldown = 0.0f;
	float m_respawnTimer = 0.0f;
	float m_invulnTimer = 0.0f;

	std::optional<ActivePowerUp> m_activePowerUp;
};
