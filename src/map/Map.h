#pragma once
#include "map/Obstacle.h"
#include "game/Arena.h"
#include "game/PowerUp.h"
#include "core/Vec2.h"
#include <vector>
#include <string>

struct PowerUpSpawnPoint {
	Vec2 position;
	PowerUpType type = PowerUpType::AutoAim;
};

class Map {
public:
	Map();

	void SetName(const std::string& name) { m_name = name; }
	const std::string& GetName() const { return m_name; }

	void SetBounds(float width, float height) { m_width = width; m_height = height; }
	float GetWidth() const { return m_width; }
	float GetHeight() const { return m_height; }

	// Obstacles
	std::vector<Obstacle>& GetObstacles() { return m_obstacles; }
	const std::vector<Obstacle>& GetObstacles() const { return m_obstacles; }
	void AddObstacle(const Obstacle& obs) { m_obstacles.push_back(obs); }
	void RemoveObstacle(int index);

	// Spawn points
	std::vector<SpawnPoint>& GetSpawnPoints() { return m_spawnPoints; }
	const std::vector<SpawnPoint>& GetSpawnPoints() const { return m_spawnPoints; }
	void AddSpawnPoint(const SpawnPoint& sp) { m_spawnPoints.push_back(sp); }

	// Power-up spawn points
	std::vector<PowerUpSpawnPoint>& GetPowerUpSpawns() { return m_powerUpSpawns; }
	const std::vector<PowerUpSpawnPoint>& GetPowerUpSpawns() const { return m_powerUpSpawns; }
	void AddPowerUpSpawn(const PowerUpSpawnPoint& ps) { m_powerUpSpawns.push_back(ps); }

	// Default game mode for this map
	GameModeSettings& GetDefaultMode() { return m_defaultMode; }
	const GameModeSettings& GetDefaultMode() const { return m_defaultMode; }

	// Create a default map
	static Map CreateDefault();

private:
	std::string m_name = "Untitled";
	float m_width = 1000.0f;
	float m_height = 1000.0f;
	std::vector<Obstacle> m_obstacles;
	std::vector<SpawnPoint> m_spawnPoints;
	std::vector<PowerUpSpawnPoint> m_powerUpSpawns;
	GameModeSettings m_defaultMode;
};
