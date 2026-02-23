#pragma once
#include "map/Map.h"
#include "map/MapSerializer.h"
#include "render/IRenderer.h"
#include "core/Vec2.h"
#include <SDL.h>
#include <string>

class MapEditor {
public:
	enum class Tool {
		Select,
		DrawObstacle,
		PlaceSpawn,
		PlacePowerUp,
		Delete,
		PlaceBountySpawn
	};

	MapEditor();

	void Init(Map* map, const std::string& filePath = "");
	void HandleEvent(const SDL_Event& event, float worldWidth, float worldHeight,
					 int windowWidth, int windowHeight);
	void Update(float dt);
	void Render(IRenderer& renderer) const;

	Map* GetMap() { return m_map; }
	bool HasUnsavedChanges() const { return m_dirty; }

private:
	Vec2 ScreenToWorld(int sx, int sy, float worldW, float worldH,
					   int winW, int winH) const;

	void HandleMouseDown(Vec2 worldPos, int button);
	void HandleMouseUp(Vec2 worldPos, int button);
	void HandleMouseMove(Vec2 worldPos);
	void HandleKeyDown(SDL_Keycode key);

	void SelectAtPoint(Vec2 pos);
	void StartDrawingObstacle(Vec2 pos);
	void AddVertexToObstacle(Vec2 pos);
	void FinishObstacle();
	void CancelObstacle();
	void DeleteSelected();

	void SnapToGrid(Vec2& pos) const;

	void RenderGrid(IRenderer& renderer) const;
	void RenderToolInfo(IRenderer& renderer) const;

	Map* m_map = nullptr;
	std::string m_filePath;
	Tool m_currentTool = Tool::Select;
	bool m_dirty = false;

	// Grid
	bool m_snapToGrid = true;
	float m_gridSize = 25.0f;

	// Selection
	int m_selectedObstacle = -1;

	// Drawing
	std::vector<Vec2> m_drawingVertices;
	bool m_isDrawing = false;

	// Dragging
	bool m_isDragging = false;
	Vec2 m_dragOffset;

	// Mouse
	Vec2 m_mouseWorldPos;

	// Power-up placement type
	int m_powerUpTypeIndex = 0;

	// Undo (single level)
	Map m_undoState;
	bool m_hasUndo = false;
};
