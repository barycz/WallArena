#include "map/Map.h"
#include <cmath>

static constexpr float PI = 3.14159265f;

Map::Map() = default;

void Map::RemoveObstacle(int index) {
	if (index >= 0 && index < static_cast<int>(m_obstacles.size())) {
		m_obstacles.erase(m_obstacles.begin() + index);
	}
}

Map Map::CreateDefault() {
	Map map;
	map.SetName("Default Arena");
	map.SetBounds(1000.0f, 1000.0f);

	float w = map.GetWidth();
	float h = map.GetHeight();
	float cx = w * 0.5f;
	float cy = h * 0.5f;

	// Center cross obstacle
	float crossSize = 40.0f;
	float crossLen = 80.0f;
	Obstacle cross({
		{cx - crossSize, cy - crossLen},
		{cx + crossSize, cy - crossLen},
		{cx + crossSize, cy - crossSize},
		{cx + crossLen, cy - crossSize},
		{cx + crossLen, cy + crossSize},
		{cx + crossSize, cy + crossSize},
		{cx + crossSize, cy + crossLen},
		{cx - crossSize, cy + crossLen},
		{cx - crossSize, cy + crossSize},
		{cx - crossLen, cy + crossSize},
		{cx - crossLen, cy - crossSize},
		{cx - crossSize, cy - crossSize}
	}, Color(180, 180, 180));
	cross.SetTag("center_pillar");
	map.AddObstacle(cross);

	// Corner blocks
	float cornerOffset = 200.0f;
	float blockSize = 50.0f;
	Vec2 corners[] = {
		{cornerOffset, cornerOffset},
		{w - cornerOffset, cornerOffset},
		{w - cornerOffset, h - cornerOffset},
		{cornerOffset, h - cornerOffset}
	};
	for (const auto& c : corners) {
		Obstacle block({
			{c.x - blockSize, c.y - blockSize},
			{c.x + blockSize, c.y - blockSize},
			{c.x + blockSize, c.y + blockSize},
			{c.x - blockSize, c.y + blockSize}
		}, Color(150, 150, 150));
		map.AddObstacle(block);
	}

	// Spawn points around the arena
	float margin = 80.0f;
	float rx = (w - margin * 2.0f) * 0.4f;
	float ry = (h - margin * 2.0f) * 0.4f;
	for (int i = 0; i < 8; ++i) {
		float angle = (2.0f * PI * i) / 8.0f;
		SpawnPoint sp;
		sp.position = {cx + std::cos(angle) * rx, cy + std::sin(angle) * ry};
		sp.angle = angle + PI;
		map.AddSpawnPoint(sp);
	}

	// Power-up spawns in diamond
	float puOffset = 150.0f;
	map.AddPowerUpSpawn({{cx, cy - puOffset}, PowerUpType::AutoAim});
	map.AddPowerUpSpawn({{cx + puOffset, cy}, PowerUpType::HomingRockets});
	map.AddPowerUpSpawn({{cx, cy + puOffset}, PowerUpType::Shield});
	map.AddPowerUpSpawn({{cx - puOffset, cy}, PowerUpType::RapidFire});

	// Default game mode
	map.GetDefaultMode().mode = GameMode::Deathmatch;
	map.GetDefaultMode().fragLimit = 10;

	return map;
}
