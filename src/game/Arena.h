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
	Deathmatch,
	LastManStanding,
	Hunt
};

struct MutatorSettings {
	bool balancing = false;
	bool billiard = false;
};

struct GameModeSettings {
	GameMode mode = GameMode::Deathmatch;
	int fragLimit = 10;
	float timeLimit = 300.0f;   // seconds (5 min)
	int respawnLives = 5;       // for LastManStanding
	int huntScore = 10;         // points to win in Hunt mode
	MutatorSettings mutators;
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
	void CheckTankCollisions();
	void CheckBulletObstacleCollisions();
	void CheckBountyHits();
	void CheckPowerUpPickups();
	void UpdateAutoAim(float dt);
	void HandleRespawns();
	void CheckRoundEnd();
	void InitPowerUpSpawns();
	SpawnPoint GetSpawnPoint() const;
	Vec2 FindNearestEnemy(int playerIndex) const;
	void RenderHUD(IRenderer& renderer) const;
	void RenderDeathEffect(IRenderer& renderer, Vec2 pos, Color color, float timer) const;
	int GetKillLeaderIndex() const;

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

	// Bounty system for Hunt mode
	struct BountySystem {
		std::vector<Vec2> spawnPoints;
		std::vector<int> scores;        // per-player hunt points
		Vec2 position;
		Vec2 nextSpawnPos;
		int health = 0;
		bool active = false;
		float respawnTimer = 0.0f;

		static constexpr int MAX_HEALTH = 3;
		static constexpr float RESPAWN_DELAY = 5.0f;
		static constexpr float TARGET_RADIUS = 15.0f;

		void Init(const std::vector<Vec2>& spawns, int playerCount);
		void SpawnTarget();
		void SelectNextSpawn();
		void Render(IRenderer& renderer) const;
		void RenderHint(IRenderer& renderer, float timer) const;
	};
	BountySystem m_bounty;
};
