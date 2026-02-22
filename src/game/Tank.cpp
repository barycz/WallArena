#include "game/Tank.h"
#include <cmath>

Tank::Tank() = default;

Tank::Tank(int playerIndex, Vec2 spawnPos, float spawnAngle, Color color)
	: m_playerIndex(playerIndex)
	, m_position(spawnPos)
	, m_angle(spawnAngle)
	, m_cannonAngle(spawnAngle)
	, m_color(color)
	, m_alive(true)
	, m_health(MAX_HEALTH)
{
}

void Tank::Update(float dt, const PlayerInput& input) {
	if (!m_alive) {
		m_respawnTimer -= dt;
		return;
	}

	// Invulnerability countdown
	if (m_invulnTimer > 0.0f) {
		m_invulnTimer -= dt;
	}

	// Fire cooldown
	if (m_fireCooldown > 0.0f) {
		m_fireCooldown -= dt;
	}

	// Power-up tick
	UpdatePowerUp(dt);

	// Rotation
	m_angle += input.turnRight * TURN_SPEED * dt;

	// Sync cannon to body when auto-aim is NOT active
	if (!HasAutoAim()) {
		m_cannonAngle = m_angle;
	}

	// Movement (forward = along body angle direction)
	Vec2 forward = Vec2::FromAngle(m_angle);
	m_position += forward * (input.moveForward * MOVE_SPEED * dt);
}

void Tank::Render(IRenderer& renderer) const {
	if (!m_alive) return;

	// Blink when invulnerable
	if (m_invulnTimer > 0.0f) {
		int blink = static_cast<int>(m_invulnTimer * 10.0f);
		if (blink % 2 == 0) return; // skip every other frame
	}

	// Draw body as a rotated rectangle
	Vec2 forward = Vec2::FromAngle(m_angle);  // body direction
	auto poly = GetBodyPolygon();
	renderer.DrawPolyline(poly, m_color, true);

	// Draw cannon (a line from center in cannon direction)
	Vec2 cannonDir = Vec2::FromAngle(m_cannonAngle);
	Vec2 cannonBase = m_position + cannonDir * BODY_HALF_H * 0.5f;
	Vec2 cannonTip = GetCannonTip();
	renderer.DrawLine(cannonBase, cannonTip, m_color);

	// Small circle at cannon tip
	renderer.DrawCircle(cannonTip, 3.0f, m_color, 6);

	// Health indicator: small ticks on top of tank
	Vec2 right = Vec2::FromAngle(m_angle + 3.14159265f * 0.5f);
	Vec2 healthBase = m_position - forward * BODY_HALF_H * 0.8f;
	float totalWidth = (MAX_HEALTH - 1) * 6.0f;
	for (int i = 0; i < m_health; ++i) {
		Vec2 pos = healthBase + right * (-totalWidth * 0.5f + i * 6.0f);
		renderer.DrawCircle(pos, 2.0f, Color::Green(), 4);
	}

	// Shield visual: outer ring
	if (HasShield()) {
		renderer.DrawCircle(m_position, BODY_HALF_H + 5.0f, Color(100, 200, 255), 12);
	}

	// Active power-up indicator dot
	if (m_activePowerUp.has_value() && !m_activePowerUp->IsExpired()) {
		Color pc = PowerUp::GetColor(m_activePowerUp->type);
		renderer.DrawCircle(m_position - forward * (BODY_HALF_H + 8.0f), 3.0f, pc, 5);
	}
}

Vec2 Tank::GetCannonTip() const {
	Vec2 cannonDir = Vec2::FromAngle(m_cannonAngle);
	return m_position + cannonDir * CANNON_LENGTH;
}

bool Tank::CanFire() const {
	return m_alive && m_fireCooldown <= 0.0f;
}

void Tank::ResetFireCooldown() {
	m_fireCooldown = FIRE_COOLDOWN * GetFireCooldownMultiplier();
}

void Tank::TakeDamage(int amount) {
	if (!m_alive || m_invulnTimer > 0.0f) return;

	// Shield absorbs hit
	if (HasShield()) {
		ConsumeShield();
		return;
	}

	m_health -= amount;
	if (m_health <= 0) {
		Kill();
	}
}

void Tank::Kill() {
	m_alive = false;
	m_health = 0;
	m_respawnTimer = RESPAWN_DELAY;
	m_activePowerUp.reset();
	deaths++;
}

void Tank::Respawn(Vec2 pos, float angle) {
	m_position = pos;
	m_angle = angle;
	m_cannonAngle = angle;
	m_alive = true;
	m_health = MAX_HEALTH;
	m_fireCooldown = 0.0f;
	m_invulnTimer = INVULN_TIME;
	m_respawnTimer = 0.0f;
	m_activePowerUp.reset();
}

std::vector<Vec2> Tank::GetBodyPolygon() const {
	Vec2 forward = Vec2::FromAngle(m_angle);
	Vec2 right = Vec2::FromAngle(m_angle + 3.14159265f * 0.5f);

	Vec2 fl = m_position + forward * BODY_HALF_H - right * BODY_HALF_W;
	Vec2 fr = m_position + forward * BODY_HALF_H + right * BODY_HALF_W;
	Vec2 br = m_position - forward * BODY_HALF_H + right * BODY_HALF_W;
	Vec2 bl = m_position - forward * BODY_HALF_H - right * BODY_HALF_W;

	return {fl, fr, br, bl};
}

// --- Power-up methods ---

void Tank::ApplyPowerUp(PowerUpType type) {
	ActivePowerUp pu;
	pu.type = type;
	pu.duration = PowerUp::GetDuration(type);
	pu.uses = PowerUp::GetUses(type);
	m_activePowerUp = pu;
}

void Tank::UpdatePowerUp(float dt) {
	if (!m_activePowerUp.has_value()) return;
	auto& pu = m_activePowerUp.value();

	if (pu.duration > 0.0f) {
		pu.duration -= dt;
	}

	if (pu.IsExpired()) {
		m_activePowerUp.reset();
	}
}

bool Tank::HasPowerUp(PowerUpType type) const {
	return m_activePowerUp.has_value() && m_activePowerUp->type == type && !m_activePowerUp->IsExpired();
}

bool Tank::HasShield() const {
	return HasPowerUp(PowerUpType::Shield);
}

void Tank::ConsumeShield() {
	if (HasShield()) {
		m_activePowerUp.reset();
	}
}

bool Tank::HasAutoAim() const {
	return HasPowerUp(PowerUpType::AutoAim);
}

float Tank::GetFireCooldownMultiplier() const {
	if (HasPowerUp(PowerUpType::RapidFire)) return 0.3f;
	return 1.0f;
}

int Tank::GetHomingRocketUses() const {
	if (m_activePowerUp.has_value() && m_activePowerUp->type == PowerUpType::HomingRockets) {
		return m_activePowerUp->uses;
	}
	return 0;
}

void Tank::ConsumeHomingRocket() {
	if (m_activePowerUp.has_value() && m_activePowerUp->type == PowerUpType::HomingRockets) {
		m_activePowerUp->uses--;
	}
}
