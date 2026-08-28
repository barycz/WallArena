#include "core/Game.h"
#include "render/SDLRenderer.h"
#include "render/ILDARenderer.h"
#include "map/MapSerializer.h"
#include "render/VectorFont.h"
#include <SDL.h>
#include <algorithm>
#include <cstring>
#include <filesystem>

Game::Game() = default;
Game::~Game() { Shutdown(); }

bool Game::Init() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
		SDL_Log("SDL_Init failed: %s", SDL_GetError());
		return false;
	}

	m_sdlRenderer = std::make_unique<SDLRenderer>();
	if (!m_sdlRenderer->Init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
		SDL_Log("SDLRenderer::Init failed");
		return false;
	}
	m_sdlRenderer->SetWorldBounds(WORLD_WIDTH, WORLD_HEIGHT);
	m_renderers.push_back(m_sdlRenderer.get());

	// ILDA renderer (always created, writes file on shutdown)
	m_ildaRenderer = std::make_unique<ILDARenderer>();
	m_ildaRenderer->Init(WINDOW_WIDTH, WINDOW_HEIGHT);
	m_ildaRenderer->SetWorldBounds(WORLD_WIDTH, WORLD_HEIGHT);
	m_ildaRenderer->SetOutputPath("output.ild");
	// Uncomment to enable dual output:
	// m_renderers.push_back(m_ildaRenderer.get());

	// Network renderer: broadcast vector frames over TCP for external hardware
	// backends (oscilloscope XY driver, laser DAC, ...).  Listens on
	// 127.0.0.1:9600; see WALLARENA_NET_PORT / _ANY / _FPS.  Set the port to 0
	// to disable.  Skips its draw calls entirely while nothing is connected.
	m_netRenderer = std::make_unique<NetRenderer>();
	if (m_netRenderer->GetPort() != 0 && m_netRenderer->Init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
		m_netRenderer->SetWorldBounds(WORLD_WIDTH, WORLD_HEIGHT);
		m_renderers.push_back(m_netRenderer.get());
	} else {
		m_netRenderer.reset();
	}

	m_input.Init();
	m_running = true;
	m_state = GameState::Menu;

	ScanMaps();
	LoadSelectedMap();

	return true;
}

void Game::Shutdown() {
	m_renderers.clear();
	m_netRenderer.reset();
	m_ildaRenderer.reset();
	m_sdlRenderer.reset();
	SDL_Quit();
}

void Game::Run() {
	const float FIXED_DT = 1.0f / 60.0f;
	Uint64 prevTicks = SDL_GetPerformanceCounter();
	float accumulator = 0.0f;

	while (m_running) {
		Uint64 nowTicks = SDL_GetPerformanceCounter();
		float frameDt = static_cast<float>(nowTicks - prevTicks) /
						static_cast<float>(SDL_GetPerformanceFrequency());
		prevTicks = nowTicks;

		if (frameDt > 0.25f) frameDt = 0.25f;
		accumulator += frameDt;

		ProcessEvents();

		while (accumulator >= FIXED_DT) {
			Update(FIXED_DT);
			accumulator -= FIXED_DT;
		}

		Render();
	}
}

void Game::ProcessEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			m_running = false;
			m_state = GameState::Quit;
			return;
		}

		if (event.type == SDL_KEYDOWN) {
			switch (m_state) {
				case GameState::Menu: {
					constexpr int MENU_ITEMS = 7;
					// Row layout: 0=START, 1=MODE, 2=BALANCE, 3=BILLIARD, 4=MAP, 5=EDITOR, 6=QUIT
					if (event.key.keysym.sym == SDLK_UP) {
						m_menuSelection = (m_menuSelection - 1 + MENU_ITEMS) % MENU_ITEMS;
					} else if (event.key.keysym.sym == SDLK_DOWN) {
						m_menuSelection = (m_menuSelection + 1) % MENU_ITEMS;
					} else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT) {
						if (m_menuSelection == 1) {
							// Cycle game mode (4 modes)
							int dir = (event.key.keysym.sym == SDLK_LEFT) ? 3 : 1;
							int m = (static_cast<int>(m_modeSettings.mode) + dir) % 4;
							m_modeSettings.mode = static_cast<GameMode>(m);
						} else if (m_menuSelection == 2) {
							// Toggle balance mutator
							m_modeSettings.mutators.balancing = !m_modeSettings.mutators.balancing;
						} else if (m_menuSelection == 3) {
							// Toggle billiard mutator
							m_modeSettings.mutators.billiard = !m_modeSettings.mutators.billiard;
						} else if (m_menuSelection == 4) {
							// Cycle map
							int n = static_cast<int>(m_mapFiles.size());
							int dir = (event.key.keysym.sym == SDLK_LEFT) ? -1 : 1;
							m_mapIndex = (m_mapIndex + dir + n) % n;
							LoadSelectedMap();
						}
					} else if (event.key.keysym.sym == SDLK_RETURN) {
						if (m_menuSelection == 0) {
							StartNewRound();
						} else if (m_menuSelection == 5) {
							m_mapEditor.Init(&m_currentMap, m_mapFiles[m_mapIndex]);
							m_state = GameState::Editor;
						} else if (m_menuSelection == 6) {
							m_running = false;
							m_state = GameState::Quit;
						}
					} else if (event.key.keysym.sym == SDLK_ESCAPE) {
						m_running = false;
						m_state = GameState::Quit;
					}
					break;
				}

				case GameState::Playing:
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						m_state = GameState::Menu;
					}
					break;

				case GameState::RoundOver:
					if (event.key.keysym.sym == SDLK_RETURN ||
						event.key.keysym.sym == SDLK_ESCAPE) {
						m_state = GameState::Menu;
					}
					break;

				case GameState::Editor:
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						ScanMaps();
						LoadSelectedMap();
						m_state = GameState::Menu;
						break;
					}
					// Fall through to forward to editor below
					break;

				case GameState::Quit:
					break;
			}
		}

		// Forward events to map editor when in editor state
		if (m_state == GameState::Editor) {
			int ww, wh;
			SDL_GetWindowSize(m_sdlRenderer->GetWindow(), &ww, &wh);
			m_mapEditor.HandleEvent(event, WORLD_WIDTH, WORLD_HEIGHT, ww, wh);
		}

		m_input.Update(event);
	}
}

void Game::ScanMaps() {
	m_mapFiles.clear();
	m_mapNames.clear();

	// Always include the built-in default as first entry
	m_mapFiles.push_back("");
	m_mapNames.push_back("DEFAULT");

	namespace fs = std::filesystem;
	fs::path mapDir = "assets/maps";
	if (!fs::exists(mapDir)) {
		fs::create_directories(mapDir);
	}

	if (fs::is_directory(mapDir)) {
		std::vector<fs::path> paths;
		for (const auto& entry : fs::directory_iterator(mapDir)) {
			if (entry.is_regular_file() && entry.path().extension() == ".map") {
				paths.push_back(entry.path());
			}
		}
		std::sort(paths.begin(), paths.end());
		for (const auto& p : paths) {
			m_mapFiles.push_back(p.string());
			m_mapNames.push_back(p.stem().string());
		}
	}

	// Clamp index
	if (m_mapIndex >= static_cast<int>(m_mapFiles.size())) {
		m_mapIndex = 0;
	}
}

void Game::LoadSelectedMap() {
	if (m_mapIndex <= 0 || m_mapIndex >= static_cast<int>(m_mapFiles.size())) {
		// Index 0 = built-in default
		m_currentMap = Map::CreateDefault();
	} else {
		if (!MapSerializer::Load(m_currentMap, m_mapFiles[m_mapIndex])) {
			m_currentMap = Map::CreateDefault();
		}
	}
	m_modeSettings = m_currentMap.GetDefaultMode();
}

void Game::StartNewRound() {
	m_arena.SetGameMode(m_modeSettings);
	m_arena.InitFromMap(m_currentMap, m_input.GetPlayerCount());
	m_state = GameState::Playing;
}

void Game::Update(float dt) {
	m_input.PollState();

	switch (m_state) {
		case GameState::Menu:
			break;

		case GameState::Playing:
			m_arena.Update(dt, m_input);
			if (m_arena.IsRoundOver()) {
				m_state = GameState::RoundOver;
				m_roundOverTimer = ROUND_OVER_DISPLAY;
			}
			break;

		case GameState::RoundOver:
			m_roundOverTimer -= dt;
			if (m_roundOverTimer <= 0.0f) {
				m_state = GameState::Menu;
			}
			break;

		case GameState::Editor:
			m_mapEditor.Update(dt);
			break;

		case GameState::Quit:
			break;
	}
}

void Game::Render() {
	for (IRenderer* renderer : m_renderers) {
		renderer->Begin();

		switch (m_state) {
			case GameState::Menu: {
				Color white = Color::White();
				Color dim(100, 100, 100);
				Color highlight = Color::Yellow();
				float cx = WORLD_WIDTH * 0.5f;

				// Title
				VectorFont::DrawTextCentered(*renderer, "WALL ARENA", cx, 150, 6.0f, white);

				// Decorative line under title
				float titleW = VectorFont::MeasureWidth("WALL ARENA", 6.0f);
				renderer->DrawLine({cx - titleW * 0.5f, 200}, {cx + titleW * 0.5f, 200}, dim);

				// Menu items
				constexpr int MENU_ITEMS = 7;
				const char* modeNames[] = {"TIME LIMIT", "DEATHMATCH", "LAST MAN STANDING", "HUNT"};
				std::string mapName = m_mapNames.empty() ? "DEFAULT" : m_mapNames[m_mapIndex];
				for (auto& ch : mapName) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

				std::string balanceStr = std::string("BALANCE: ") + (m_modeSettings.mutators.balancing ? "ON" : "OFF");
				std::string billiardStr = std::string("BILLIARD: ") + (m_modeSettings.mutators.billiard ? "ON" : "OFF");

				std::string labels[] = {
					"START GAME",
					modeNames[static_cast<int>(m_modeSettings.mode)],
					balanceStr,
					billiardStr,
					mapName,
					"MAP EDITOR",
					"QUIT"
				};
				float menuY = 260.0f;
				float itemSpacing = 42.0f;

				for (int i = 0; i < MENU_ITEMS; ++i) {
					Color c = (i == m_menuSelection) ? highlight : dim;
					// Mutator rows use green when ON
					if ((i == 2 && m_modeSettings.mutators.balancing) ||
						(i == 3 && m_modeSettings.mutators.billiard)) {
						c = (i == m_menuSelection) ? highlight : Color(0, 180, 0);
					}
					float y = menuY + i * itemSpacing;
					float sc = (i >= 2 && i <= 3) ? 2.5f : 3.5f; // smaller for mutators

					VectorFont::DrawTextCentered(*renderer, labels[i], cx, y, sc, c);

					// Selection arrow
					if (i == m_menuSelection) {
						float textW = VectorFont::MeasureWidth(labels[i], sc);
						float ax = cx - textW * 0.5f - 20.0f;
						float bx = cx + textW * 0.5f + 20.0f;
						float ay = y + sc * 3.5f;
						renderer->DrawLine({ax, ay - 8}, {ax + 10, ay}, highlight);
						renderer->DrawLine({ax + 10, ay}, {ax, ay + 8}, highlight);
						renderer->DrawLine({bx, ay - 8}, {bx - 10, ay}, highlight);
						renderer->DrawLine({bx - 10, ay}, {bx, ay + 8}, highlight);
					}

					// Left/right arrows for cyclable/toggleable rows
					if (i == 1 || i == 2 || i == 3 || i == 4) {
						float sc2 = (i >= 2 && i <= 3) ? 2.5f : 3.5f;
						float textW = VectorFont::MeasureWidth(labels[i], sc2);
						float lx = cx - textW * 0.5f - 30.0f;
						float rx = cx + textW * 0.5f + 20.0f;
						float ay = y + sc2 * 3.5f;
						renderer->DrawLine({lx + 10, ay - 8}, {lx, ay}, white);
						renderer->DrawLine({lx, ay}, {lx + 10, ay + 8}, white);
						renderer->DrawLine({rx, ay - 8}, {rx + 10, ay}, white);
						renderer->DrawLine({rx + 10, ay}, {rx, ay + 8}, white);
					}
				}

				// Player count indicator
				int pc = m_input.GetPlayerCount();
				VectorFont::DrawTextCentered(*renderer, "PLAYERS", cx, 570, 2.5f, dim);
				for (int i = 0; i < pc; ++i) {
					Color c = Color::FromIndex(i);
					float px = cx - (pc * 15.0f) + i * 30.0f;
					renderer->DrawRect({px, 600}, {20, 20}, c);
				}

				// Controls hint
				VectorFont::DrawTextCentered(*renderer, "UP/DOWN - SELECT   ENTER - CONFIRM   ESC - QUIT",
											 cx, 950, 1.5f, Color(50, 50, 50));
				break;
			}

			case GameState::Playing: {
				m_arena.Render(*renderer);
				break;
			}

			case GameState::RoundOver: {
				m_arena.Render(*renderer);

				// Overlay
				float cx = WORLD_WIDTH * 0.5f;
				float cy = WORLD_HEIGHT * 0.5f;
				int winner = m_arena.GetWinnerIndex();
				if (winner >= 0 && winner < m_arena.GetPlayerCount()) {
					Color wc = m_arena.GetTanks()[winner].GetColor();
					renderer->DrawCircle({cx, cy}, 100.0f, wc, 32);
					renderer->DrawCircle({cx, cy}, 105.0f, wc, 32);

					VectorFont::DrawTextCentered(*renderer, "WINNER", cx, cy - 30, 5.0f, wc);
					std::string pLabel = "PLAYER " + std::to_string(winner + 1);
					VectorFont::DrawTextCentered(*renderer, pLabel, cx, cy + 20, 3.0f, wc);
				} else {
					VectorFont::DrawTextCentered(*renderer, "DRAW", cx, cy - 15, 5.0f, Color::White());
				}
				VectorFont::DrawTextCentered(*renderer, "PRESS ENTER", cx, cy + 70, 2.0f, Color(100, 100, 100));
				break;
			}

			case GameState::Editor: {
				m_mapEditor.Render(*renderer);
				break;
			}

			case GameState::Quit:
				break;
		}

		renderer->End();
	}
}
