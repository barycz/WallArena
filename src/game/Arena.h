#pragma once
#include "game/Tank.h"
#include "game/Projectile.h"
#include "game/PowerUp.h"
#include "game/Collision.h"
#include "input/InputManager.h"
#include "render/IRenderer.h"
#include <vector>
#include <functional>

class Map;
class Obstacle;

enum class GameMode {
	TimeLimit,
	FragLimit,
	LastManStanding
};

struct GameModeSettings {
	GameMode mode = GameMode::FragLimit;
	int fragLimit = 10;
	float timeLimit = 300.0f;   // seconds (5 min)
	int respawnLives = 5;       // for LastManStanding
};

struct SpawnPoint {
	Vec2 position;
	float angle = 0.0f;
};

class Arena {
public:
	Arena();

	void Init(float width, float height, int playerCount);
	void InitFromMap(const Map& map, int playerCount);
	void SetGameMode(const GameModeSettings& settings);

	void Update(float dt, const InputManager& input);
	void Render(IRenderer& renderer) const;

	bool IsRoundOver() const { return m_roundOver; }
	int GetWinnerIndex() const { return m_winnerIndex; }

	void AddPlayer();
	void RemovePlayer(int index);
	int GetPlayerCount() const { return static_cast<int>(m_tanks.size()); }

	const std::vector<Tank>& GetTanks() const { return m_tanks; }

	float GetWidth() const { return m_width; }
	float GetHeight() const { return m_height; }

private:
	void SpawnBullet(int playerIndex, bool homing = false);
	void UpdateProjectiles(float dt);
	void CheckBulletCollisions();
	void CheckBoundaryCollisions();
	void CheckObstacleCollisions();
	void CheckBulletObstacleCollisions();
	void CheckPowerUpPickups();
	void UpdateAutoAim(float dt);
	void HandleRespawns();
	void CheckRoundEnd();
	void InitPowerUpSpawns();
	SpawnPoint GetSpawnPoint() const;
	Vec2 FindNearestEnemy(int playerIndex) const;
	void RenderHUD(IRenderer& renderer) const;
	void RenderDeathEffect(IRenderer& renderer, Vec2 pos, Color color, float timer) const;

	float m_width = 1000.0f;
	float m_height = 1000.0f;

	std::vector<Tank> m_tanks;
	std::vector<Projectile> m_projectiles;
	std::vector<SpawnPoint> m_spawnPoints;
	std::vector<Obstacle> m_obstacles;

	// Death effects
	struct DeathEffect {
		Vec2 position;
		Color color;
		float timer;
	};
	std::vector<DeathEffect> m_deathEffects;

	GameModeSettings m_modeSettings;
	float m_roundTimer = 0.0f;
	bool m_roundOver = false;
	int m_winnerIndex = -1;

	// Power-ups
	std::vector<PowerUp> m_powerUps;

	// Homing projectile tracking: projectile index -> target player index
	struct HomingInfo {
		int targetIndex = -1;
	};
	std::vector<HomingInfo> m_homingInfo; // parallel to m_projectiles

	// LastManStanding: remaining lives per player
	std::vector<int> m_livesRemaining;
};
