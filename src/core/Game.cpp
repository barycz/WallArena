#include "core/Game.h"
#include "render/SDLRenderer.h"
#include "render/ILDARenderer.h"
#include "map/MapSerializer.h"
#include "render/VectorFont.h"
#include <SDL.h>
#include <algorithm>
#include <cstring>

Game::Game() = default;
Game::~Game() { Shutdown(); }

bool Game::Init() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
		SDL_Log("SDL_Init failed: %s", SDL_GetError());
		return false;
	}

	m_sdlRenderer = std::make_unique<SDLRenderer>();
	if (!m_sdlRenderer->Init(1280, 720)) {
		SDL_Log("SDLRenderer::Init failed");
		return false;
	}
	m_sdlRenderer->SetWorldBounds(WORLD_WIDTH, WORLD_HEIGHT);
	m_renderers.push_back(m_sdlRenderer.get());

	// ILDA renderer (always created, writes file on shutdown)
	m_ildaRenderer = std::make_unique<ILDARenderer>();
	m_ildaRenderer->Init(1280, 720);
	m_ildaRenderer->SetWorldBounds(WORLD_WIDTH, WORLD_HEIGHT);
	m_ildaRenderer->SetOutputPath("output.ild");
	// Uncomment to enable dual output:
	// m_renderers.push_back(m_ildaRenderer.get());

	m_input.Init();
	m_running = true;
	m_state = GameState::Menu;

	// Try loading default map from file, fall back to generated default
	if (!MapSerializer::Load(m_currentMap, "assets/maps/Default Arena.map")) {
		m_currentMap = Map::CreateDefault();
	}
	m_modeSettings = m_currentMap.GetDefaultMode();

	return true;
}

void Game::Shutdown() {
	m_renderers.clear();
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
				case GameState::Menu:
					if (event.key.keysym.sym == SDLK_UP) {
						m_menuSelection = (m_menuSelection - 1 + 4) % 4;
					} else if (event.key.keysym.sym == SDLK_DOWN) {
						m_menuSelection = (m_menuSelection + 1) % 4;
					} else if (event.key.keysym.sym == SDLK_LEFT) {
						// Cycle game mode backward
						if (m_menuSelection == 1) {
							int m = (static_cast<int>(m_modeSettings.mode) + 2) % 3;
							m_modeSettings.mode = static_cast<GameMode>(m);
						}
					} else if (event.key.keysym.sym == SDLK_RIGHT) {
						// Cycle game mode forward
						if (m_menuSelection == 1) {
							int m = (static_cast<int>(m_modeSettings.mode) + 1) % 3;
							m_modeSettings.mode = static_cast<GameMode>(m);
						}
					} else if (event.key.keysym.sym == SDLK_RETURN) {
						if (m_menuSelection == 0) {
							StartNewRound();
						} else if (m_menuSelection == 2) {
							m_mapEditor.Init(&m_currentMap);
							m_state = GameState::Editor;
						} else if (m_menuSelection == 3) {
							m_running = false;
							m_state = GameState::Quit;
						}
					} else if (event.key.keysym.sym == SDLK_ESCAPE) {
						m_running = false;
						m_state = GameState::Quit;
					}
					break;

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
				const char* modeNames[] = {"TIME LIMIT", "FRAG LIMIT", "LAST MAN STANDING"};
				const char* labels[] = {
					"START GAME",
					modeNames[static_cast<int>(m_modeSettings.mode)],
					"MAP EDITOR",
					"QUIT"
				};
				float menuY = 300.0f;
				float itemSpacing = 65.0f;

				for (int i = 0; i < 4; ++i) {
					Color c = (i == m_menuSelection) ? highlight : dim;
					float y = menuY + i * itemSpacing;

					VectorFont::DrawTextCentered(*renderer, labels[i], cx, y, 3.5f, c);

					// Selection arrow
					if (i == m_menuSelection) {
						float textW = VectorFont::MeasureWidth(labels[i], 3.5f);
						float ax = cx - textW * 0.5f - 20.0f;
						float ay = y + 12.0f;
						renderer->DrawLine({ax, ay - 8}, {ax + 10, ay}, highlight);
						renderer->DrawLine({ax + 10, ay}, {ax, ay + 8}, highlight);
					}

					// Left/right arrows for mode selection
					if (i == 1) {
						float textW = VectorFont::MeasureWidth(labels[i], 3.5f);
						float lx = cx - textW * 0.5f - 30.0f;
						float rx = cx + textW * 0.5f + 20.0f;
						float ay = y + 12.0f;
						renderer->DrawLine({lx + 10, ay - 8}, {lx, ay}, white);
						renderer->DrawLine({lx, ay}, {lx + 10, ay + 8}, white);
						renderer->DrawLine({rx, ay - 8}, {rx + 10, ay}, white);
						renderer->DrawLine({rx + 10, ay}, {rx, ay + 8}, white);
					}
				}

				// Player count indicator
				int pc = m_input.GetPlayerCount();
				VectorFont::DrawTextCentered(*renderer, "PLAYERS", cx, 600, 2.5f, dim);
				for (int i = 0; i < pc; ++i) {
					Color c = Color::FromIndex(i);
					float px = cx - (pc * 15.0f) + i * 30.0f;
					renderer->DrawRect({px, 630}, {20, 20}, c);
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
