#pragma once
#include <memory>
#include <vector>
#include <string>
#include "render/IRenderer.h"
#include "input/InputManager.h"
#include "game/Arena.h"
#include "map/Map.h"
#include "editor/MapEditor.h"
#include "render/ILDARenderer.h"
#include "render/NetRenderer.h"

enum class GameState {
	Menu,
	Playing,
	RoundOver,
	Editor,
	Quit
};

class Game {
public:
	Game();
	~Game();

	bool Init();
	void Run();
	void Shutdown();

private:
	void ProcessEvents();
	void Update(float dt);
	void Render();

	void StartNewRound();
	void ScanMaps();
	void LoadSelectedMap();

	GameState m_state = GameState::Menu;
	bool m_running = false;

	std::vector<IRenderer*> m_renderers;
	std::unique_ptr<class SDLRenderer> m_sdlRenderer;
	std::unique_ptr<ILDARenderer> m_ildaRenderer;
	std::unique_ptr<NetRenderer> m_netRenderer;
	InputManager m_input;
	Arena m_arena;
	GameModeSettings m_modeSettings;
	Map m_currentMap;
	MapEditor m_mapEditor;

	// Menu selection state
	int m_menuSelection = 0;

	// Map management
	std::vector<std::string> m_mapFiles; // full paths to .map files
	std::vector<std::string> m_mapNames; // display names
	int m_mapIndex = 0;

	// Round over display timer
	float m_roundOverTimer = 0.0f;
	static constexpr float ROUND_OVER_DISPLAY = 3.0f;

	// World constants
	static constexpr float WORLD_WIDTH = 1000.0f;
	static constexpr float WORLD_HEIGHT = 1000.0f;
};
