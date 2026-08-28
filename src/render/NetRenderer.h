#pragma once
#include "render/IRenderer.h"
#include <chrono>
#include <vector>
#include <string>
#include <cstdint>

// NetRenderer - broadcasts every rendered frame as vector geometry over TCP so
// external "display backends" (an oscilloscope XY driver, a laser DAC, a
// visualiser, ...) can draw the game on real hardware.
//
// It is a normal IRenderer: add it to Game::m_renderers next to the SDL one.
// A TCP listen socket is opened on Init(); clients may connect/disconnect at any
// time.  Frames are only serialised and sent while at least one client is
// connected -- with no client attached the draw calls are skipped outright.
// Non-blocking accept + bounded-timeout send: a client that cannot keep up is
// dropped rather than stalling the game loop.
//
// Wire format - one text line per frame, '\n' terminated, fields space
// separated, paths separated by " ; ":
//
//     F <worldW> <worldH> <numPaths> ; <r> <g> <b> <n> <x0> <y0> ... ; ...
//
// Coordinates are world units (same space as IRenderer draw calls, Y down).
// Each path is a pen-down polyline; the client blanks the beam between paths.
//
// The listen socket is bound to loopback unless WALLARENA_NET_ANY is set, and
// the number of simultaneous clients is capped, so the worst case a peer can
// impose on the render thread is bounded.
//
// POSIX sockets only.  On Windows this compiles to a no-op renderer.
class NetRenderer : public IRenderer {
public:
	NetRenderer();
	~NetRenderer() override;

	bool Init(int windowWidth, int windowHeight) override;
	void Shutdown() override;

	void Begin() override;
	void End() override;

	void DrawLine(Vec2 a, Vec2 b, Color c) override;
	void DrawPolyline(const std::vector<Vec2>& pts, Color c, bool closed = false) override;
	void DrawCircle(Vec2 center, float radius, Color c, int segments = 16) override;
	void DrawRect(Vec2 topLeft, Vec2 size, Color c) override;

	void SetWorldBounds(float width, float height) override;

	void SetPort(uint16_t port) { m_port = port; }
	uint16_t GetPort() const { return m_port; }
	int GetClientCount() const { return static_cast<int>(m_clients.size()); }

private:
	struct Path {
		Color c;
		std::vector<Vec2> pts;
		bool fromLine = false; // eligible for DrawLine chaining
	};

	Path& NewPath(Color c, bool fromLine);
	void PollAccept();
	void Broadcast(const std::string& msg);

	float m_worldWidth = 1000.0f;
	float m_worldHeight = 1000.0f;

	uint16_t m_port = 9600;
	bool m_bindAny = false; // false: listen on loopback only
	int m_listenFd = -1;
	std::vector<int> m_clients;

	std::vector<Path> m_paths;
	bool m_active = false; // this frame has a listener and is inside the rate cap

	// Cap broadcast rate (the game may render far faster than vsync).
	std::chrono::duration<double> m_minFramePeriod{1.0 / 60.0};
	std::chrono::steady_clock::time_point m_lastSend{};
};
