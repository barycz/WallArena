#include "game/Arena.h"
#include "map/Map.h"
#include "map/Obstacle.h"
#include "render/VectorFont.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

static constexpr float PI = 3.14159265f;

Arena::Arena() = default;

void Arena::Init(float width, float height, int playerCount) {
	m_width = width;
	m_height = height;
	m_tanks.clear();
	m_projectiles.clear();
	m_deathEffects.clear();
	m_roundOver = false;
	m_winnerIndex = -1;
	m_roundTimer = 0.0f;
	m_powerUps.clear();
	m_homingInfo.clear();

	// Generate spawn points evenly around the arena
	m_spawnPoints.clear();
	float margin = 80.0f;
	float cx = width * 0.5f;
	float cy = height * 0.5f;
	float rx = (width - margin * 2.0f) * 0.4f;
	float ry = (height - margin * 2.0f) * 0.4f;
	for (int i = 0; i < 8; ++i) {
		float angle = (2.0f * PI * i) / 8.0f;
		SpawnPoint sp;
		sp.position = {cx + std::cos(angle) * rx, cy + std::sin(angle) * ry};
		sp.angle = angle + PI; // face center
		m_spawnPoints.push_back(sp);
	}

	// Create tanks
	for (int i = 0; i < playerCount; ++i) {
		AddPlayer();
	}

	// Init lives for LMS
	m_livesRemaining.clear();
	m_livesRemaining.assign(playerCount, m_modeSettings.respawnLives);

	// Init power-up spawn locations
	InitPowerUpSpawns();
}

void Arena::InitFromMap(const Map& map, int playerCount) {
	m_width = map.GetWidth();
	m_height = map.GetHeight();
	m_tanks.clear();
	m_projectiles.clear();
	m_deathEffects.clear();
	m_obstacles.clear();
	m_powerUps.clear();
	m_homingInfo.clear();
	m_roundOver = false;
	m_winnerIndex = -1;
	m_roundTimer = 0.0f;

	// Copy obstacles from map
	m_obstacles = map.GetObstacles();

	// Copy spawn points from map
	m_spawnPoints.clear();
	for (const auto& sp : map.GetSpawnPoints()) {
		m_spawnPoints.push_back(sp);
	}
	// If map has no spawn points, generate defaults
	if (m_spawnPoints.empty()) {
		float cx = m_width * 0.5f;
		float cy = m_height * 0.5f;
		float rx = (m_width - 160.0f) * 0.4f;
		float ry = (m_height - 160.0f) * 0.4f;
		for (int i = 0; i < 8; ++i) {
			float angle = (2.0f * PI * i) / 8.0f;
			SpawnPoint sp;
			sp.position = {cx + std::cos(angle) * rx, cy + std::sin(angle) * ry};
			sp.angle = angle + PI;
			m_spawnPoints.push_back(sp);
		}
	}

	// Create tanks
	for (int i = 0; i < playerCount; ++i) {
		AddPlayer();
	}

	// Init lives for LMS
	m_livesRemaining.clear();
	m_livesRemaining.assign(playerCount, m_modeSettings.respawnLives);

	// Power-up spawns from map
	m_powerUps.clear();
	for (const auto& ps : map.GetPowerUpSpawns()) {
		m_powerUps.emplace_back(ps.position, ps.type);
	}
	// If map has no power-up spawns, use defaults
	if (m_powerUps.empty()) {
		InitPowerUpSpawns();
	}

	// Init bounty system for Hunt mode
	if (m_modeSettings.mode == GameMode::Hunt) {
		std::vector<Vec2> bountySpawns = map.GetBountySpawns();
		// If no bounty spawns defined, generate defaults
		if (bountySpawns.empty()) {
			float cx = m_width * 0.5f;
			float cy = m_height * 0.5f;
			float off = 150.0f;
			bountySpawns.push_back({cx, cy});
			bountySpawns.push_back({cx - off, cy - off});
			bountySpawns.push_back({cx + off, cy - off});
			bountySpawns.push_back({cx + off, cy + off});
			bountySpawns.push_back({cx - off, cy + off});
		}
		m_bounty.Init(bountySpawns, playerCount);
	}
}

void Arena::SetGameMode(const GameModeSettings& settings) {
	m_modeSettings = settings;
}

void Arena::AddPlayer() {
	int idx = static_cast<int>(m_tanks.size());
	SpawnPoint sp = GetSpawnPoint();
	Color col = Color::FromIndex(idx);
	m_tanks.emplace_back(idx, sp.position, sp.angle, col);

	if (m_modeSettings.mode == GameMode::LastManStanding) {
		m_livesRemaining.push_back(m_modeSettings.respawnLives);
	}
}

void Arena::RemovePlayer(int index) {
	if (index < 0 || index >= static_cast<int>(m_tanks.size())) return;
	m_tanks.erase(m_tanks.begin() + index);
	if (index < static_cast<int>(m_livesRemaining.size())) {
		m_livesRemaining.erase(m_livesRemaining.begin() + index);
	}
	// Re-index remaining tanks
	for (int i = 0; i < static_cast<int>(m_tanks.size()); ++i) {
		// Player index is baked in the tank, but for simplicity we keep original indices
	}
}

SpawnPoint Arena::GetSpawnPoint() const {
	if (m_spawnPoints.empty()) {
		return {{m_width * 0.5f, m_height * 0.5f}, 0.0f};
	}
	int r = std::rand() % static_cast<int>(m_spawnPoints.size());
	return m_spawnPoints[r];
}

void Arena::Update(float dt, const InputManager& input) {
	if (m_roundOver) return;

	m_roundTimer += dt;

	// Auto-aim: rotate tanks with auto-aim toward nearest enemy
	UpdateAutoAim(dt);

	// Update tanks
	for (auto& tank : m_tanks) {
		const PlayerInput& pi = input.GetPlayerInput(tank.GetPlayerIndex());
		tank.Update(dt, pi);

		// Fire
		if (pi.firePressed && tank.CanFire()) {
			bool homing = tank.GetHomingRocketUses() > 0;
			SpawnBullet(tank.GetPlayerIndex(), homing);
			if (homing) tank.ConsumeHomingRocket();
			tank.ResetFireCooldown();
			// Balancing: kill leader gets 50% slower fire rate
			if (m_modeSettings.mutators.balancing &&
				tank.GetPlayerIndex() == GetKillLeaderIndex()) {
				tank.AddFireCooldown(Tank::FIRE_COOLDOWN * 0.5f);
			}
		}
	}

	// Keep tanks in bounds
	CheckBoundaryCollisions();

	// Obstacle collisions for tanks
	CheckObstacleCollisions();

	// Tank vs tank collisions
	CheckTankCollisions();

	// Update & collide projectiles
	UpdateProjectiles(dt);
	CheckBulletCollisions();
	CheckBulletObstacleCollisions();

	// Bounty system (Hunt mode)
	if (m_modeSettings.mode == GameMode::Hunt) {
		if (!m_bounty.active) {
			m_bounty.respawnTimer -= dt;
			if (m_bounty.respawnTimer <= 0.0f) {
				m_bounty.SpawnTarget();
			}
		}
		CheckBountyHits();
	}

	// Power-up pickups
	for (auto& pu : m_powerUps) pu.Update(dt);
	CheckPowerUpPickups();

	// Handle respawns
	HandleRespawns();

	// Update death effects
	for (auto& fx : m_deathEffects) {
		fx.timer -= dt;
	}
	m_deathEffects.erase(
		std::remove_if(m_deathEffects.begin(), m_deathEffects.end(),
						[](const DeathEffect& fx) { return fx.timer <= 0.0f; }),
		m_deathEffects.end());

	// Check win conditions
	CheckRoundEnd();
}

void Arena::SpawnBullet(int playerIndex, bool homing) {
	if (playerIndex < 0 || playerIndex >= static_cast<int>(m_tanks.size())) return;
	const Tank& tank = m_tanks[playerIndex];

	Vec2 pos = tank.GetCannonTip();
	Vec2 vel = tank.GetCannonForward() * Projectile::SPEED;
	Color col = tank.GetColor();

	// Homing rockets are orange-tinted and slightly slower
	if (homing) {
		col = Color(255, 128, 0);
		vel = tank.GetCannonForward() * (Projectile::SPEED * 0.7f);
	}

	m_projectiles.emplace_back(pos, vel, playerIndex, col);

	HomingInfo hi;
	if (homing) {
		// Find nearest enemy as initial target
		Vec2 nearest = FindNearestEnemy(playerIndex);
		// Find the tank index at that position
		float bestDist = 1e9f;
		for (const auto& t : m_tanks) {
			if (t.GetPlayerIndex() == playerIndex || !t.IsAlive()) continue;
			float d = Vec2::Distance(t.GetPosition(), pos);
			if (d < bestDist) {
				bestDist = d;
				hi.targetIndex = t.GetPlayerIndex();
			}
		}
	}
	m_homingInfo.push_back(hi);
}

void Arena::UpdateProjectiles(float dt) {
	static constexpr float HOMING_TURN_RATE = 3.0f; // radians/sec

	for (size_t i = 0; i < m_projectiles.size(); ++i) {
		auto& proj = m_projectiles[i];

		// Homing steering
		if (i < m_homingInfo.size() && m_homingInfo[i].targetIndex >= 0) {
			int ti = m_homingInfo[i].targetIndex;
			if (ti < static_cast<int>(m_tanks.size()) && m_tanks[ti].IsAlive()) {
				Vec2 toTarget = m_tanks[ti].GetPosition() - proj.GetPosition();
				Vec2 velDir = proj.GetVelocity().Normalized();
				Vec2 desiredDir = toTarget.Normalized();
				// Blend current direction toward target
				float cross = velDir.Cross(desiredDir);
				float turnAmount = HOMING_TURN_RATE * dt;
				if (cross > 0) turnAmount = std::min(turnAmount, cross);
				else turnAmount = std::max(-turnAmount, cross);
				Vec2 newDir = velDir.Rotated(turnAmount);
				proj.SetVelocity(newDir * proj.GetVelocity().Length());
			}
		}

		proj.Update(dt);

		// Bounce off arena walls
		Vec2 pos = proj.GetPosition();
		Vec2 vel = proj.GetVelocity();
		bool bounced = false;

		if (pos.x < 0.0f || pos.x > m_width) {
			vel = {-vel.x, vel.y};
			pos.x = std::max(0.0f, std::min(m_width, pos.x));
			bounced = true;
		}
		if (pos.y < 0.0f || pos.y > m_height) {
			vel = {vel.x, -vel.y};
			pos.y = std::max(0.0f, std::min(m_height, pos.y));
			bounced = true;
		}

		if (bounced) {
			proj.SetPosition(pos);
			proj.SetVelocity(vel);
			proj.IncrementBounce();
		}
	}

	// Remove dead projectiles (and their homing info)
	// Build keep-list
	std::vector<Projectile> keptProj;
	std::vector<HomingInfo> keptHoming;
	for (size_t i = 0; i < m_projectiles.size(); ++i) {
		if (m_projectiles[i].IsAlive()) {
			keptProj.push_back(m_projectiles[i]);
			if (i < m_homingInfo.size())
				keptHoming.push_back(m_homingInfo[i]);
			else
				keptHoming.push_back({});
		}
	}
	m_projectiles = std::move(keptProj);
	m_homingInfo = std::move(keptHoming);
}

void Arena::CheckBulletCollisions() {
	for (auto& proj : m_projectiles) {
		if (!proj.IsAlive()) continue;

		for (auto& tank : m_tanks) {
			if (!tank.IsAlive()) continue;
			if (tank.GetPlayerIndex() == proj.GetOwnerIndex()) continue;

			auto poly = tank.GetBodyPolygon();
			if (Collision::PointInPolygon(proj.GetPosition(), poly)) {
				// Billiard: projectile must have bounced at least once to deal damage
				if (m_modeSettings.mutators.billiard && proj.GetBounceCount() == 0) {
					// Visual feedback but no damage
					m_deathEffects.push_back({proj.GetPosition(), Color(80, 80, 80), 0.15f});
					proj.Kill();
					break;
				}
				// Small hit effect in victim's color
				m_deathEffects.push_back({proj.GetPosition(), tank.GetColor(), 0.15f});
				tank.TakeDamage(1);
				proj.Kill();

				if (!tank.IsAlive()) {
					// Credit kill to shooter
					for (auto& t : m_tanks) {
						if (t.GetPlayerIndex() == proj.GetOwnerIndex()) {
							t.kills++;
							break;
						}
					}
					// Balancing: kill leader gets longer respawn
					if (m_modeSettings.mutators.balancing &&
						tank.GetPlayerIndex() == GetKillLeaderIndex()) {
						tank.SetRespawnDelay(Tank::RESPAWN_DELAY * 1.5f);
					}
					// Death effect
					m_deathEffects.push_back({tank.GetPosition(), tank.GetColor(), 0.8f});
				}
				break;
			}
		}
	}
}

void Arena::CheckBoundaryCollisions() {
	for (auto& tank : m_tanks) {
		if (!tank.IsAlive()) continue;

		Vec2 pos = tank.GetPosition();
		float r = Tank::COLLISION_RADIUS;

		if (pos.x < r)            pos.x = r;
		if (pos.x > m_width - r)  pos.x = m_width - r;
		if (pos.y < r)            pos.y = r;
		if (pos.y > m_height - r) pos.y = m_height - r;

		tank.SetPosition(pos);
	}
}

void Arena::HandleRespawns() {
	for (auto& tank : m_tanks) {
		if (tank.IsAlive()) continue;
		if (tank.GetRespawnTimer() > 0.0f) continue;

		// Check if allowed to respawn
		if (m_modeSettings.mode == GameMode::LastManStanding) {
			int idx = tank.GetPlayerIndex();
			if (idx < static_cast<int>(m_livesRemaining.size())) {
				if (m_livesRemaining[idx] <= 0) continue; // no lives left
				m_livesRemaining[idx]--;
			}
		}

		SpawnPoint sp = GetSpawnPoint();
		tank.Respawn(sp.position, sp.angle);
	}
}

void Arena::CheckRoundEnd() {
	switch (m_modeSettings.mode) {
		case GameMode::TimeLimit: {
			if (m_roundTimer >= m_modeSettings.timeLimit) {
				m_roundOver = true;
				// Winner = most kills
				int bestKills = -1;
				for (const auto& t : m_tanks) {
					if (t.kills > bestKills) {
						bestKills = t.kills;
						m_winnerIndex = t.GetPlayerIndex();
					}
				}
			}
			break;
		}
		case GameMode::FragLimit: {
			for (const auto& t : m_tanks) {
				if (t.kills >= m_modeSettings.fragLimit) {
					m_roundOver = true;
					m_winnerIndex = t.GetPlayerIndex();
					return;
				}
			}
			break;
		}
		case GameMode::LastManStanding: {
			int aliveCount = 0;
			int lastAlive = -1;
			for (const auto& t : m_tanks) {
				int idx = t.GetPlayerIndex();
				bool canRespawn = idx < static_cast<int>(m_livesRemaining.size()) &&
								  m_livesRemaining[idx] > 0;
				if (t.IsAlive() || canRespawn) {
					aliveCount++;
					lastAlive = idx;
				}
			}
			if (aliveCount <= 1 && m_tanks.size() > 1) {
				m_roundOver = true;
				m_winnerIndex = lastAlive;
			}
			break;
		}
		case GameMode::Hunt: {
			for (int i = 0; i < static_cast<int>(m_bounty.scores.size()); ++i) {
				if (m_bounty.scores[i] >= m_modeSettings.huntScore) {
					m_roundOver = true;
					m_winnerIndex = i;
					return;
				}
			}
			break;
		}
	}
}

void Arena::CheckObstacleCollisions() {
	float r = Tank::COLLISION_RADIUS;

	for (auto& tank : m_tanks) {
		if (!tank.IsAlive()) continue;
		Vec2 tankPos = tank.GetPosition();

		for (const auto& obs : m_obstacles) {
			const auto& verts = obs.GetVertices();
			if (verts.size() < 3) continue;

			int n = static_cast<int>(verts.size());

			// If center is inside polygon, push out along nearest edge normal
			if (Collision::PointInPolygon(tankPos, verts)) {
				float bestDist = 1e9f;
				Vec2 bestPush = {0, 0};
				for (int i = 0; i < n; ++i) {
					Vec2 a = verts[i];
					Vec2 b = verts[(i + 1) % n];
					Vec2 edge = b - a;
					Vec2 normal = Vec2(-edge.y, edge.x).Normalized();
					float dist = (tankPos - a).Dot(normal);
					if (std::abs(dist) < bestDist) {
						bestDist = std::abs(dist);
						bestPush = normal * (-dist - r - 1.0f);
					}
				}
				tankPos = tankPos + bestPush;
				tank.SetPosition(tankPos);
				continue;
			}

			// Circle vs polygon edges: push out if within radius
			for (int i = 0; i < n; ++i) {
				Vec2 a = verts[i];
				Vec2 b = verts[(i + 1) % n];
				Vec2 ab = b - a;
				float len2 = ab.Dot(ab);
				if (len2 < 1e-8f) continue;

				// Project tank center onto edge, clamp to segment
				float t = (tankPos - a).Dot(ab) / len2;
				t = std::max(0.0f, std::min(1.0f, t));
				Vec2 closest = a + ab * t;

				Vec2 diff = tankPos - closest;
				float dist2 = diff.Dot(diff);
				if (dist2 < r * r && dist2 > 1e-8f) {
					float dist = std::sqrt(dist2);
					Vec2 pushDir = diff * (1.0f / dist);
					tankPos = closest + pushDir * (r + 0.5f);
					tank.SetPosition(tankPos);
				}
			}
		}
	}
}

void Arena::CheckTankCollisions() {
	float r = Tank::COLLISION_RADIUS;
	float minDist = r * 2.0f;

	for (size_t i = 0; i < m_tanks.size(); ++i) {
		if (!m_tanks[i].IsAlive()) continue;

		for (size_t j = i + 1; j < m_tanks.size(); ++j) {
			if (!m_tanks[j].IsAlive()) continue;

			Vec2 posA = m_tanks[i].GetPosition();
			Vec2 posB = m_tanks[j].GetPosition();
			Vec2 diff = posA - posB;
			float dist2 = diff.Dot(diff);

			if (dist2 < minDist * minDist && dist2 > 1e-8f) {
				float dist = std::sqrt(dist2);
				Vec2 dir = diff * (1.0f / dist);
				float overlap = (minDist - dist) * 0.5f + 0.5f;
				m_tanks[i].SetPosition(posA + dir * overlap);
				m_tanks[j].SetPosition(posB - dir * overlap);
			} else if (dist2 <= 1e-8f) {
				// Exactly overlapping: nudge apart
				m_tanks[i].SetPosition(posA + Vec2(1.0f, 0.0f));
				m_tanks[j].SetPosition(posB - Vec2(1.0f, 0.0f));
			}
		}
	}
}

void Arena::CheckBulletObstacleCollisions() {
	for (auto& proj : m_projectiles) {
		if (!proj.IsAlive()) continue;

		for (const auto& obs : m_obstacles) {
			const auto& verts = obs.GetVertices();
			if (verts.size() < 3) continue;

			if (Collision::PointInPolygon(proj.GetPosition(), verts)) {
				if (m_modeSettings.mutators.billiard) {
					// Reflect off nearest edge
					Vec2 p = proj.GetPosition();
					float bestDist2 = 1e18f;
					Vec2 bestNormal = {0, -1};
					Vec2 bestClosest = p;

					for (size_t i = 0; i < verts.size(); ++i) {
						Vec2 a = verts[i];
						Vec2 b = verts[(i + 1) % verts.size()];
						Vec2 ab = b - a;
						float len2 = ab.Dot(ab);
						if (len2 < 1e-8f) continue;
						float t = (p - a).Dot(ab) / len2;
						t = std::max(0.0f, std::min(1.0f, t));
						Vec2 closest = a + ab * t;
						float d2 = (p - closest).Dot(p - closest);
						if (d2 < bestDist2) {
							bestDist2 = d2;
							bestClosest = closest;
							// Edge normal (outward)
							Vec2 edge = ab.Normalized();
							bestNormal = {-edge.y, edge.x};
							// Ensure normal points away from polygon center
							Vec2 mid = (a + b) * 0.5f;
							Vec2 toP = p - mid;
							if (toP.Dot(bestNormal) < 0) bestNormal = bestNormal * -1.0f;
						}
					}

					// Reflect velocity
					Vec2 vel = proj.GetVelocity();
					float dot = vel.Dot(bestNormal);
					Vec2 reflected = vel - bestNormal * (2.0f * dot);
					proj.SetVelocity(reflected);
					// Push projectile out of obstacle
					proj.SetPosition(bestClosest + bestNormal * 2.0f);
					proj.IncrementBounce();
					m_deathEffects.push_back({bestClosest, Color(200, 200, 200), 0.3f});
				} else {
					m_deathEffects.push_back({proj.GetPosition(), Color(200, 200, 200), 0.3f});
					proj.Kill();
				}
				break;
			}
		}
	}
}

void Arena::Render(IRenderer& renderer) const {
	// Arena boundary
	renderer.DrawRect({0, 0}, {m_width, m_height}, Color::White());

	// Obstacles
	for (const auto& obs : m_obstacles) {
		obs.Render(renderer);
	}

	// Tanks
	for (const auto& tank : m_tanks) {
		tank.Render(renderer);
	}

	// Projectiles
	for (const auto& proj : m_projectiles) {
		proj.Render(renderer);
	}

	// Death effects (burst of lines)
	for (const auto& fx : m_deathEffects) {
		RenderDeathEffect(renderer, fx.position, fx.color, fx.timer);
	}

	// Power-ups
	for (const auto& pu : m_powerUps) {
		pu.Render(renderer);
	}

	// Bounty target (Hunt mode)
	if (m_modeSettings.mode == GameMode::Hunt) {
		m_bounty.Render(renderer);
		m_bounty.RenderHint(renderer, m_roundTimer);
	}

	// HUD
	RenderHUD(renderer);
}

void Arena::RenderDeathEffect(IRenderer& renderer, Vec2 pos, Color color, float timer) const {
	// Three sizes: tiny hit (<=0.15), small obstacle (<=0.3), large death (<=0.8)
	float maxTime;
	int numLines;
	float baseRadius, expandRadius;
	if (timer <= 0.15f) {
		maxTime = 0.15f; numLines = 4; baseRadius = 4.0f; expandRadius = 10.0f;
	} else if (timer <= 0.3f) {
		maxTime = 0.3f; numLines = 5; baseRadius = 8.0f; expandRadius = 20.0f;
	} else {
		maxTime = 0.8f; numLines = 8; baseRadius = 20.0f; expandRadius = 60.0f;
	}
	float progress = 1.0f - (timer / maxTime); // 0 -> 1
	float radius = baseRadius + progress * expandRadius;

	Color c = color;
	c.a = static_cast<uint8_t>(255 * (1.0f - progress));

	for (int i = 0; i < numLines; ++i) {
		float angle = (2.0f * PI * i) / numLines + progress * 0.5f;
		Vec2 inner = pos + Vec2::FromAngle(angle) * (radius * 0.3f);
		Vec2 outer = pos + Vec2::FromAngle(angle) * radius;
		renderer.DrawLine(inner, outer, c);
	}
}

void Arena::InitPowerUpSpawns() {
	m_powerUps.clear();
	// Place 4 power-ups in a diamond pattern around center
	float cx = m_width * 0.5f;
	float cy = m_height * 0.5f;
	float offset = 150.0f;

	Vec2 positions[] = {
		{cx, cy - offset},
		{cx + offset, cy},
		{cx, cy + offset},
		{cx - offset, cy}
	};
	PowerUpType types[] = {
		PowerUpType::AutoAim,
		PowerUpType::HomingRockets,
		PowerUpType::Shield,
		PowerUpType::RapidFire
	};

	for (int i = 0; i < 4; ++i) {
		m_powerUps.emplace_back(positions[i], types[i]);
	}
}

void Arena::CheckPowerUpPickups() {
	for (auto& pu : m_powerUps) {
		if (!pu.IsActive()) continue;

		for (auto& tank : m_tanks) {
			if (!tank.IsAlive()) continue;

			float dist = Vec2::Distance(tank.GetPosition(), pu.GetPosition());
			if (dist < pu.GetRadius() + Tank::BODY_HALF_W) {
				tank.ApplyPowerUp(pu.GetType());
				pu.Collect();
				break;
			}
		}
	}
}

void Arena::UpdateAutoAim(float dt) {
	for (auto& tank : m_tanks) {
		if (!tank.IsAlive() || !tank.HasAutoAim()) continue;

		Vec2 target = FindNearestEnemy(tank.GetPlayerIndex());
		if (target.x < -9000.0f) continue; // no enemy found

		// Smoothly rotate toward nearest enemy
		Vec2 toTarget = target - tank.GetPosition();
		float desiredAngle = toTarget.Angle();
		float currentAngle = tank.GetCannonAngle();

		// Shortest angular difference
		float diff = desiredAngle - currentAngle;
		while (diff > PI) diff -= 2.0f * PI;
		while (diff < -PI) diff += 2.0f * PI;

		float maxTurn = Tank::TURN_SPEED * 3.0f * dt;
		if (std::abs(diff) < maxTurn) {
			tank.SetCannonAngle(desiredAngle);
		} else {
			float sign = (diff > 0.0f) ? 1.0f : -1.0f;
			tank.SetCannonAngle(currentAngle + sign * maxTurn);
		}
	}
}

Vec2 Arena::FindNearestEnemy(int playerIndex) const {
	Vec2 myPos = {-99999.0f, -99999.0f};
	for (const auto& t : m_tanks) {
		if (t.GetPlayerIndex() == playerIndex && t.IsAlive()) {
			myPos = t.GetPosition();
			break;
		}
	}
	if (myPos.x < -9000.0f) return myPos;

	Vec2 nearest = {-99999.0f, -99999.0f};
	float bestDist = 1e9f;
	for (const auto& t : m_tanks) {
		if (t.GetPlayerIndex() == playerIndex || !t.IsAlive()) continue;
		float d = Vec2::Distance(t.GetPosition(), myPos);
		if (d < bestDist) {
			bestDist = d;
			nearest = t.GetPosition();
		}
	}
	return nearest;
}

void Arena::RenderHUD(IRenderer& renderer) const {
	// Scoreboard at top of screen
	float xStart = 15.0f;
	float y = 12.0f;
	float scale = 1.8f;

	for (const auto& tank : m_tanks) {
		Color c = tank.GetColor();
		int idx = tank.GetPlayerIndex();

		// "P1 - N" label
		std::string label = "P" + std::to_string(idx + 1) + " - ";
		VectorFont::DrawText(renderer, label, {xStart, y}, scale, c);
		float lx = xStart + VectorFont::MeasureWidth(label, scale);

		// Score display: hunt score in Hunt mode, kill count otherwise
		std::string scoreStr;
		if (m_modeSettings.mode == GameMode::Hunt &&
			idx < static_cast<int>(m_bounty.scores.size())) {
			scoreStr = std::to_string(m_bounty.scores[idx]);
		} else {
			scoreStr = std::to_string(tank.kills);
		}
		VectorFont::DrawText(renderer, scoreStr, {lx, y}, scale, c);
		lx += VectorFont::MeasureWidth(scoreStr, scale) + 8.0f;

		// Health pips
		for (int h = 0; h < tank.GetHealth(); ++h) {
			renderer.DrawCircle({lx + h * 8.0f, y + 6.0f}, 2.5f, Color::Green(), 4);
		}
		lx += Tank::MAX_HEALTH * 8.0f + 5.0f;

		// Lives remaining (LMS mode)
		if (m_modeSettings.mode == GameMode::LastManStanding) {
			if (idx < static_cast<int>(m_livesRemaining.size())) {
				std::string livesStr = "x" + std::to_string(m_livesRemaining[idx]);
				VectorFont::DrawText(renderer, livesStr, {lx, y}, scale * 0.8f, c);
			}
		}

		// Active power-up name
		auto pu = tank.GetActivePowerUp();
		if (pu.has_value() && !pu->IsExpired()) {
			Color pc = PowerUp::GetColor(pu->type);
			std::string puName = PowerUp::GetName(pu->type);
			VectorFont::DrawText(renderer, puName, {lx + 30.0f, y}, scale * 0.7f, pc);
		}

		xStart += 200.0f;
	}

	// Timer (for TimeLimit mode)
	if (m_modeSettings.mode == GameMode::TimeLimit) {
		float remaining = m_modeSettings.timeLimit - m_roundTimer;
		if (remaining < 0) remaining = 0;
		int mins = static_cast<int>(remaining) / 60;
		int secs = static_cast<int>(remaining) % 60;
		std::string timeStr = std::to_string(mins) + ":"
			+ (secs < 10 ? "0" : "") + std::to_string(secs);
		Color tc = (remaining < 30.0f) ? Color::Red() : Color::Yellow();
		VectorFont::DrawTextCentered(renderer, timeStr, m_width * 0.5f, 5.0f, 2.5f, tc);
	}

	// Frag limit indicator
	if (m_modeSettings.mode == GameMode::FragLimit) {
		std::string fragStr = "FIRST TO " + std::to_string(m_modeSettings.fragLimit);
		VectorFont::DrawTextCentered(renderer, fragStr, m_width * 0.5f, 5.0f, 1.5f, Color(60, 60, 60));
	}

	// Hunt score indicator
	if (m_modeSettings.mode == GameMode::Hunt) {
		std::string huntStr = "FIRST TO " + std::to_string(m_modeSettings.huntScore);
		VectorFont::DrawTextCentered(renderer, huntStr, m_width * 0.5f, 5.0f, 1.5f, Color(60, 60, 60));
	}
}

int Arena::GetKillLeaderIndex() const {
	int bestKills = 0;
	int leaderIdx = -1;
	bool tied = false;
	for (const auto& t : m_tanks) {
		if (t.kills > bestKills) {
			bestKills = t.kills;
			leaderIdx = t.GetPlayerIndex();
			tied = false;
		} else if (t.kills == bestKills && bestKills > 0) {
			tied = true;
		}
	}
	// No leader if tied or no kills yet
	return tied ? -1 : leaderIdx;
}

void Arena::CheckBountyHits() {
	if (!m_bounty.active) return;

	for (auto& proj : m_projectiles) {
		if (!proj.IsAlive()) continue;

		float dist = Vec2::Distance(proj.GetPosition(), m_bounty.position);
		if (dist < Arena::BountySystem::TARGET_RADIUS) {
			// Billiard gate applies to bounty too
			if (m_modeSettings.mutators.billiard && proj.GetBounceCount() == 0) {
				m_deathEffects.push_back({proj.GetPosition(), Color(80, 80, 80), 0.15f});
				proj.Kill();
				continue;
			}

			m_deathEffects.push_back({proj.GetPosition(), Color(255, 200, 0), 0.15f});
			proj.Kill();
			m_bounty.health--;

			if (m_bounty.health <= 0) {
				// Credit hunt point to shooter
				int owner = proj.GetOwnerIndex();
				if (owner >= 0 && owner < static_cast<int>(m_bounty.scores.size())) {
					m_bounty.scores[owner]++;
				}
				m_deathEffects.push_back({m_bounty.position, Color(255, 200, 0), 0.8f});
				m_bounty.active = false;
				m_bounty.respawnTimer = Arena::BountySystem::RESPAWN_DELAY;
				m_bounty.SelectNextSpawn();
			}
			break;
		}
	}
}

// --- BountySystem methods ---

void Arena::BountySystem::Init(const std::vector<Vec2>& spawns, int playerCount) {
	spawnPoints = spawns;
	scores.assign(playerCount, 0);
	health = 0;
	active = false;
	respawnTimer = 2.0f; // short initial delay before first target
	SelectNextSpawn();
}

void Arena::BountySystem::SpawnTarget() {
	position = nextSpawnPos;
	health = MAX_HEALTH;
	active = true;
	respawnTimer = 0.0f;
}

void Arena::BountySystem::SelectNextSpawn() {
	if (spawnPoints.empty()) return;
	int idx = std::rand() % static_cast<int>(spawnPoints.size());
	nextSpawnPos = spawnPoints[idx];
}

void Arena::BountySystem::Render(IRenderer& renderer) const {
	if (!active) return;

	Color gold(255, 200, 0);

	// Diamond shape
	float r = TARGET_RADIUS;
	Vec2 top = position + Vec2(0, -r);
	Vec2 right = position + Vec2(r, 0);
	Vec2 bottom = position + Vec2(0, r);
	Vec2 left = position + Vec2(-r, 0);
	renderer.DrawLine(top, right, gold);
	renderer.DrawLine(right, bottom, gold);
	renderer.DrawLine(bottom, left, gold);
	renderer.DrawLine(left, top, gold);

	// Inner diamond
	float r2 = r * 0.5f;
	Vec2 top2 = position + Vec2(0, -r2);
	Vec2 right2 = position + Vec2(r2, 0);
	Vec2 bottom2 = position + Vec2(0, r2);
	Vec2 left2 = position + Vec2(-r2, 0);
	renderer.DrawLine(top2, right2, gold);
	renderer.DrawLine(right2, bottom2, gold);
	renderer.DrawLine(bottom2, left2, gold);
	renderer.DrawLine(left2, top2, gold);

	// Health pips above target
	float pipY = position.y - r - 8.0f;
	float totalW = (MAX_HEALTH - 1) * 8.0f;
	for (int i = 0; i < health; ++i) {
		float px = position.x - totalW * 0.5f + i * 8.0f;
		renderer.DrawCircle({px, pipY}, 2.5f, gold, 4);
	}
}

void Arena::BountySystem::RenderHint(IRenderer& renderer, float timer) const {
	if (active) return; // no hint while target is alive

	Color hint(255, 200, 0);
	// Pulsing circle at next spawn position
	float pulse = 0.5f + 0.5f * std::sin(timer * 6.0f);
	uint8_t alpha = static_cast<uint8_t>(60 + 80 * pulse);
	Color hintC(255, 200, 0, alpha);
	renderer.DrawCircle(nextSpawnPos, TARGET_RADIUS * (1.0f + pulse * 0.3f), hintC, 8);
}
