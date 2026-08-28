#include "render/NetRenderer.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef _WIN32
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

// Linux spells "do not raise SIGPIPE on a dead peer" as a send() flag; the BSDs
// (macOS included) spell it as a per-socket option set at accept() time.
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

namespace {
constexpr float CHAIN_EPS = 0.75f; // world units: join DrawLine calls closer than this
constexpr float PI = 3.14159265358979f; // M_PI is not standard C++ (absent on MSVC)

// Parse an env var as a number, complaining instead of silently using the default.
bool EnvNumber(const char* name, double& out) {
	const char* env = std::getenv(name);
	if (!env) return false;
	char* end = nullptr;
	double v = std::strtod(env, &end);
	if (end == env || *end != '\0') {
		std::fprintf(stderr, "NetRenderer: ignoring invalid %s=\"%s\"\n", name, env);
		return false;
	}
	out = v;
	return true;
}
} // namespace

NetRenderer::NetRenderer() {
	double v = 0.0;
	if (EnvNumber("WALLARENA_NET_PORT", v)) {
		// 0 is a valid value: it means "do not open the socket at all".
		if (v >= 0.0 && v < 65536.0) {
			m_port = static_cast<uint16_t>(v);
		} else {
			std::fprintf(stderr, "NetRenderer: WALLARENA_NET_PORT out of range, using %u\n",
						 static_cast<unsigned>(m_port));
		}
	}
	if (EnvNumber("WALLARENA_NET_FPS", v)) {
		if (v > 0.0) {
			m_minFramePeriod = std::chrono::duration<double>(1.0 / v);
		} else {
			std::fprintf(stderr, "NetRenderer: WALLARENA_NET_FPS must be > 0, ignoring\n");
		}
	}
}

NetRenderer::~NetRenderer() { Shutdown(); }

#ifdef _WIN32 // ---------------------------------------------------------------

bool NetRenderer::Init(int, int) {
	std::printf("NetRenderer: disabled on this platform\n");
	return false;
}
void NetRenderer::Shutdown() {}
void NetRenderer::PollAccept() {}
void NetRenderer::Broadcast(const std::string&) {}

#else // -----------------------------------------------------------------------

bool NetRenderer::Init(int, int) {
	if (m_port == 0) {
		std::printf("NetRenderer: disabled (port 0)\n");
		return false;
	}

	m_listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (m_listenFd < 0) {
		std::perror("NetRenderer: socket");
		return false;
	}

	int one = 1;
	::setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(m_port);

	if (::bind(m_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
		std::perror("NetRenderer: bind");
		::close(m_listenFd);
		m_listenFd = -1;
		return false;
	}
	if (::listen(m_listenFd, 8) < 0) {
		std::perror("NetRenderer: listen");
		::close(m_listenFd);
		m_listenFd = -1;
		return false;
	}
	::fcntl(m_listenFd, F_SETFL, O_NONBLOCK);

	std::printf("NetRenderer: broadcasting frames on TCP port %u\n",
				static_cast<unsigned>(m_port));
	return true;
}

void NetRenderer::Shutdown() {
	for (int fd : m_clients) ::close(fd);
	m_clients.clear();
	if (m_listenFd >= 0) {
		::close(m_listenFd);
		m_listenFd = -1;
	}
}

void NetRenderer::PollAccept() {
	for (;;) {
		int fd = ::accept(m_listenFd, nullptr, nullptr);
		if (fd < 0) break; // EWOULDBLOCK / EAGAIN: no more pending clients

		int one = 1;
		::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
#ifdef SO_NOSIGPIPE
		::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
		// A client that can't absorb a frame within 100 ms is too slow -> drop it.
		timeval tv{};
		tv.tv_usec = 100000;
		::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

		m_clients.push_back(fd);
		std::printf("NetRenderer: client connected (%d total)\n",
					static_cast<int>(m_clients.size()));
	}
}

static bool SendAll(int fd, const char* p, size_t n) {
	while (n > 0) {
		ssize_t k = ::send(fd, p, n, MSG_NOSIGNAL);
		if (k <= 0) return false; // error or send timeout
		p += k;
		n -= static_cast<size_t>(k);
	}
	return true;
}

void NetRenderer::Broadcast(const std::string& msg) {
	for (size_t i = 0; i < m_clients.size();) {
		if (SendAll(m_clients[i], msg.data(), msg.size())) {
			++i;
		} else {
			std::printf("NetRenderer: client disconnected\n");
			::close(m_clients[i]);
			m_clients.erase(m_clients.begin() + static_cast<long>(i));
		}
	}
}

#endif // --------------------------------------------------------------------

void NetRenderer::Begin() {
	m_paths.clear();
}

void NetRenderer::End() {
	if (m_listenFd < 0) return;

	PollAccept();
	if (m_clients.empty()) return;

	auto now = std::chrono::steady_clock::now();
	if (now - m_lastSend < m_minFramePeriod) return;
	m_lastSend = now;

	std::string msg;
	msg.reserve(4096);
	char buf[96];

	std::snprintf(buf, sizeof(buf), "F %.1f %.1f %zu", m_worldWidth, m_worldHeight,
				  m_paths.size());
	msg += buf;

	for (const Path& p : m_paths) {
		std::snprintf(buf, sizeof(buf), " ; %d %d %d %zu", p.c.r, p.c.g, p.c.b,
					  p.pts.size());
		msg += buf;
		for (const Vec2& v : p.pts) {
			std::snprintf(buf, sizeof(buf), " %.1f %.1f", v.x, v.y);
			msg += buf;
		}
	}
	msg += '\n';

	Broadcast(msg);
}

void NetRenderer::DrawLine(Vec2 a, Vec2 b, Color c) {
	if (!m_paths.empty()) {
		Path& last = m_paths.back();
		if (last.fromLine && last.c.r == c.r && last.c.g == c.g && last.c.b == c.b &&
			Vec2::Distance(last.pts.back(), a) <= CHAIN_EPS) {
			last.pts.push_back(b);
			return;
		}
	}
	Path path;
	path.c = c;
	path.fromLine = true;
	path.pts = {a, b};
	m_paths.push_back(std::move(path));
}

void NetRenderer::DrawPolyline(const std::vector<Vec2>& pts, Color c, bool closed) {
	if (pts.size() < 2) return;
	Path path;
	path.c = c;
	path.pts = pts;
	if (closed && pts.size() > 2) path.pts.push_back(pts.front());
	m_paths.push_back(std::move(path));
}

void NetRenderer::DrawCircle(Vec2 center, float radius, Color c, int segments) {
	if (segments < 3) segments = 3;
	std::vector<Vec2> pts;
	pts.reserve(static_cast<size_t>(segments) + 1);
	for (int i = 0; i <= segments; ++i) {
		float angle = (2.0f * PI * i) / segments;
		pts.push_back(center + Vec2::FromAngle(angle) * radius);
	}
	DrawPolyline(pts, c, false);
}

void NetRenderer::DrawRect(Vec2 topLeft, Vec2 size, Color c) {
	std::vector<Vec2> pts = {
		topLeft,
		{topLeft.x + size.x, topLeft.y},
		{topLeft.x + size.x, topLeft.y + size.y},
		{topLeft.x, topLeft.y + size.y},
	};
	DrawPolyline(pts, c, true);
}

void NetRenderer::SetWorldBounds(float width, float height) {
	m_worldWidth = width;
	m_worldHeight = height;
}
