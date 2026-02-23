#include "editor/MapEditor.h"
#include "render/VectorFont.h"
#include <cmath>
#include <algorithm>

static constexpr float PI = 3.14159265f;

MapEditor::MapEditor() = default;

void MapEditor::Init(Map* map, const std::string& filePath) {
	m_map = map;
	m_filePath = filePath;
	m_currentTool = Tool::Select;
	m_dirty = false;
	m_selectedObstacle = -1;
	m_isDrawing = false;
	m_isDragging = false;
	m_drawingVertices.clear();
	m_hasUndo = false;
}

Vec2 MapEditor::ScreenToWorld(int sx, int sy, float worldW, float worldH,
							   int winW, int winH) const {
	float scaleX = static_cast<float>(winW) / worldW;
	float scaleY = static_cast<float>(winH) / worldH;
	float scale = std::min(scaleX, scaleY);

	float offsetX = (winW - worldW * scale) * 0.5f;
	float offsetY = (winH - worldH * scale) * 0.5f;

	float wx = (sx - offsetX) / scale;
	float wy = (sy - offsetY) / scale;
	return {wx, wy};
}

void MapEditor::HandleEvent(const SDL_Event& event, float worldWidth, float worldHeight,
							 int windowWidth, int windowHeight) {
	if (!m_map) return;

	if (event.type == SDL_MOUSEMOTION) {
		m_mouseWorldPos = ScreenToWorld(event.motion.x, event.motion.y,
										 worldWidth, worldHeight,
										 windowWidth, windowHeight);
		if (m_snapToGrid) SnapToGrid(m_mouseWorldPos);
		HandleMouseMove(m_mouseWorldPos);
	}
	else if (event.type == SDL_MOUSEBUTTONDOWN) {
		Vec2 pos = ScreenToWorld(event.button.x, event.button.y,
								  worldWidth, worldHeight,
								  windowWidth, windowHeight);
		if (m_snapToGrid) SnapToGrid(pos);
		HandleMouseDown(pos, event.button.button);
	}
	else if (event.type == SDL_MOUSEBUTTONUP) {
		Vec2 pos = ScreenToWorld(event.button.x, event.button.y,
								  worldWidth, worldHeight,
								  windowWidth, windowHeight);
		HandleMouseUp(pos, event.button.button);
	}
	else if (event.type == SDL_KEYDOWN) {
		HandleKeyDown(event.key.keysym.sym);
	}
}

void MapEditor::HandleMouseDown(Vec2 worldPos, int button) {
	if (button == SDL_BUTTON_LEFT) {
		switch (m_currentTool) {
			case Tool::Select:
				SelectAtPoint(worldPos);
				if (m_selectedObstacle >= 0) {
					m_isDragging = true;
					auto& obs = m_map->GetObstacles()[m_selectedObstacle];
					// Compute center for drag offset
					Vec2 center = {0, 0};
					for (const auto& v : obs.GetVertices()) center += v;
					center = center / static_cast<float>(obs.GetVertices().size());
					m_dragOffset = center - worldPos;
				}
				break;

			case Tool::DrawObstacle:
				if (!m_isDrawing) {
					StartDrawingObstacle(worldPos);
				} else {
					AddVertexToObstacle(worldPos);
				}
				break;

			case Tool::PlaceSpawn: {
				// Save undo
				m_undoState = *m_map;
				m_hasUndo = true;

				SpawnPoint sp;
				sp.position = worldPos;
				sp.angle = 0.0f;
				m_map->AddSpawnPoint(sp);
				m_dirty = true;
				break;
			}

			case Tool::PlacePowerUp: {
				m_undoState = *m_map;
				m_hasUndo = true;

				PowerUpSpawnPoint ps;
				ps.position = worldPos;
				ps.type = static_cast<PowerUpType>(m_powerUpTypeIndex % static_cast<int>(PowerUpType::Count));
				m_map->AddPowerUpSpawn(ps);
				m_dirty = true;
				break;
			}

			case Tool::Delete:
				SelectAtPoint(worldPos);
				if (m_selectedObstacle >= 0) {
					DeleteSelected();
				}
				break;
		}
	}
	else if (button == SDL_BUTTON_RIGHT) {
		if (m_isDrawing) {
			// Finish obstacle if enough vertices
			if (m_drawingVertices.size() >= 3) {
				FinishObstacle();
			} else {
				CancelObstacle();
			}
		} else {
			// Right-click to delete spawn/powerup near click
			// Check spawn points
			auto& spawns = m_map->GetSpawnPoints();
			for (auto it = spawns.begin(); it != spawns.end(); ++it) {
				if (Vec2::Distance(it->position, worldPos) < 20.0f) {
					m_undoState = *m_map;
					m_hasUndo = true;
					spawns.erase(it);
					m_dirty = true;
					return;
				}
			}
			// Check power-up spawns
			auto& pups = m_map->GetPowerUpSpawns();
			for (auto it = pups.begin(); it != pups.end(); ++it) {
				if (Vec2::Distance(it->position, worldPos) < 20.0f) {
					m_undoState = *m_map;
					m_hasUndo = true;
					pups.erase(it);
					m_dirty = true;
					return;
				}
			}
		}
	}
}

void MapEditor::HandleMouseUp(Vec2 worldPos, int button) {
	if (button == SDL_BUTTON_LEFT) {
		m_isDragging = false;
	}
}

void MapEditor::HandleMouseMove(Vec2 worldPos) {
	if (m_isDragging && m_selectedObstacle >= 0) {
		auto& obs = m_map->GetObstacles()[m_selectedObstacle];
		Vec2 center = {0, 0};
		for (const auto& v : obs.GetVertices()) center += v;
		center = center / static_cast<float>(obs.GetVertices().size());

		Vec2 newCenter = worldPos + m_dragOffset;
		Vec2 delta = newCenter - center;

		std::vector<Vec2> newVerts;
		for (const auto& v : obs.GetVertices()) {
			newVerts.push_back(v + delta);
		}
		obs.SetVertices(newVerts);
		m_dirty = true;
	}
}

void MapEditor::HandleKeyDown(SDL_Keycode key) {
	switch (key) {
		case SDLK_1: m_currentTool = Tool::Select; m_isDrawing = false; break;
		case SDLK_2: m_currentTool = Tool::DrawObstacle; break;
		case SDLK_3: m_currentTool = Tool::PlaceSpawn; break;
		case SDLK_4: m_currentTool = Tool::PlacePowerUp; break;
		case SDLK_5: m_currentTool = Tool::Delete; break;

		case SDLK_g: m_snapToGrid = !m_snapToGrid; break;

		case SDLK_TAB:
			// Cycle power-up type
			m_powerUpTypeIndex = (m_powerUpTypeIndex + 1) % static_cast<int>(PowerUpType::Count);
			break;

		case SDLK_DELETE:
		case SDLK_BACKSPACE:
			DeleteSelected();
			break;

		case SDLK_z:
			// Undo
			if (m_hasUndo && m_map) {
				*m_map = m_undoState;
				m_hasUndo = false;
				m_selectedObstacle = -1;
				m_dirty = true;
			}
			break;

		case SDLK_s:
			// Save
			if (m_map) {
				std::string path = m_filePath.empty()
					? ("assets/maps/" + m_map->GetName() + ".map")
					: m_filePath;
				MapSerializer::Save(*m_map, path);
				m_filePath = path;
				m_dirty = false;
			}
			break;

		default:
			break;
	}
}

void MapEditor::SelectAtPoint(Vec2 pos) {
	m_selectedObstacle = -1;
	if (!m_map) return;

	const auto& obstacles = m_map->GetObstacles();
	for (int i = 0; i < static_cast<int>(obstacles.size()); ++i) {
		if (obstacles[i].ContainsPoint(pos)) {
			m_selectedObstacle = i;
			return;
		}
	}
}

void MapEditor::StartDrawingObstacle(Vec2 pos) {
	m_drawingVertices.clear();
	m_drawingVertices.push_back(pos);
	m_isDrawing = true;
}

void MapEditor::AddVertexToObstacle(Vec2 pos) {
	m_drawingVertices.push_back(pos);
}

void MapEditor::FinishObstacle() {
	if (!m_map || m_drawingVertices.size() < 3) {
		CancelObstacle();
		return;
	}

	m_undoState = *m_map;
	m_hasUndo = true;

	Obstacle obs(m_drawingVertices, Color(160, 160, 160));
	m_map->AddObstacle(obs);
	m_dirty = true;

	m_drawingVertices.clear();
	m_isDrawing = false;
}

void MapEditor::CancelObstacle() {
	m_drawingVertices.clear();
	m_isDrawing = false;
}

void MapEditor::DeleteSelected() {
	if (!m_map || m_selectedObstacle < 0) return;

	m_undoState = *m_map;
	m_hasUndo = true;

	m_map->RemoveObstacle(m_selectedObstacle);
	m_selectedObstacle = -1;
	m_dirty = true;
}

void MapEditor::SnapToGrid(Vec2& pos) const {
	pos.x = std::round(pos.x / m_gridSize) * m_gridSize;
	pos.y = std::round(pos.y / m_gridSize) * m_gridSize;
}

void MapEditor::Update(float dt) {
	(void)dt;
}

void MapEditor::Render(IRenderer& renderer) const {
	if (!m_map) return;

	float w = m_map->GetWidth();
	float h = m_map->GetHeight();

	// Grid
	if (m_snapToGrid) {
		RenderGrid(renderer);
	}

	// Arena boundary
	renderer.DrawRect({0, 0}, {w, h}, Color(60, 60, 60));

	// Obstacles
	const auto& obstacles = m_map->GetObstacles();
	for (int i = 0; i < static_cast<int>(obstacles.size()); ++i) {
		Color c = obstacles[i].GetColor();
		if (i == m_selectedObstacle) {
			c = Color::Yellow(); // highlight selected
		}
		renderer.DrawPolyline(obstacles[i].GetVertices(), c, true);
	}

	// Drawing preview
	if (m_isDrawing && !m_drawingVertices.empty()) {
		Color drawColor = Color::Green();
		for (size_t i = 0; i + 1 < m_drawingVertices.size(); ++i) {
			renderer.DrawLine(m_drawingVertices[i], m_drawingVertices[i + 1], drawColor);
		}
		// Line to cursor
		renderer.DrawLine(m_drawingVertices.back(), m_mouseWorldPos, Color(0, 200, 0));
		// Closing line preview
		if (m_drawingVertices.size() >= 3) {
			renderer.DrawLine(m_mouseWorldPos, m_drawingVertices.front(), Color(0, 150, 0));
		}
		// Vertex dots
		for (const auto& v : m_drawingVertices) {
			renderer.DrawCircle(v, 3.0f, drawColor, 5);
		}
	}

	// Spawn points
	for (const auto& sp : m_map->GetSpawnPoints()) {
		renderer.DrawCircle(sp.position, 10.0f, Color::Cyan(), 6);
		Vec2 dir = Vec2::FromAngle(sp.angle);
		renderer.DrawLine(sp.position, sp.position + dir * 15.0f, Color::Cyan());
	}

	// Power-up spawn points
	for (const auto& ps : m_map->GetPowerUpSpawns()) {
		Color c = PowerUp::GetColor(ps.type);
		renderer.DrawCircle(ps.position, 12.0f, c, 8);
		renderer.DrawCircle(ps.position, 6.0f, c, 4);
	}

	// Tool info
	RenderToolInfo(renderer);
}

void MapEditor::RenderGrid(IRenderer& renderer) const {
	if (!m_map) return;
	Color gridColor(30, 30, 30);
	float w = m_map->GetWidth();
	float h = m_map->GetHeight();

	for (float x = 0; x <= w; x += m_gridSize) {
		renderer.DrawLine({x, 0}, {x, h}, gridColor);
	}
	for (float y = 0; y <= h; y += m_gridSize) {
		renderer.DrawLine({0, y}, {w, y}, gridColor);
	}
}

void MapEditor::RenderToolInfo(IRenderer& renderer) const {
	float w = m_map ? m_map->GetWidth() : 1000.0f;
	float h = m_map ? m_map->GetHeight() : 1000.0f;

	// Editor title
	VectorFont::DrawText(renderer, "MAP EDITOR", {10, 10}, 2.5f, Color(80, 80, 80));

	// Tool bar at bottom
	float y = h - 28.0f;
	float x = 10.0f;
	float toolScale = 1.5f;

	const char* toolNames[] = {"1-SELECT", "2-DRAW", "3-SPAWN", "4-POWERUP", "5-DELETE"};
	int toolIdx = static_cast<int>(m_currentTool);

	for (int i = 0; i < 5; ++i) {
		Color c = (i == toolIdx) ? Color::Yellow() : Color(50, 50, 50);
		VectorFont::DrawText(renderer, toolNames[i], {x, y}, toolScale, c);
		x += VectorFont::MeasureWidth(toolNames[i], toolScale) + 15.0f;
	}

	// Grid indicator
	Color gc = m_snapToGrid ? Color::Green() : Color(50, 50, 50);
	VectorFont::DrawText(renderer, "G-GRID", {x, y}, toolScale, gc);
	x += VectorFont::MeasureWidth("G-GRID", toolScale) + 15.0f;

	// Current power-up type (when in power-up tool)
	if (m_currentTool == Tool::PlacePowerUp) {
		PowerUpType pt = static_cast<PowerUpType>(m_powerUpTypeIndex % static_cast<int>(PowerUpType::Count));
		Color pc = PowerUp::GetColor(pt);
		const char* puName = PowerUp::GetName(pt);
		VectorFont::DrawText(renderer, puName, {x, y}, toolScale, pc);
		VectorFont::DrawText(renderer, "(TAB)", {x + VectorFont::MeasureWidth(puName, toolScale) + 5.0f, y},
							 toolScale * 0.8f, Color(50, 50, 50));
	}

	// Hints
	VectorFont::DrawText(renderer, "S-SAVE  Z-UNDO  ESC-BACK", {w - 280.0f, y}, 1.2f, Color(40, 40, 40));

	// Cursor crosshair
	Color cursorC(100, 100, 100);
	renderer.DrawLine(m_mouseWorldPos + Vec2(-8, 0), m_mouseWorldPos + Vec2(8, 0), cursorC);
	renderer.DrawLine(m_mouseWorldPos + Vec2(0, -8), m_mouseWorldPos + Vec2(0, 8), cursorC);
}
