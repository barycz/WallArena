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
#include <poll.h>
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

// Bounds on how much the render thread may ever pay for the network.  Sends are
// non-blocking; a client that has not drained a frame within SEND_BUDGET_MS is
// declared too slow and dropped, so the worst-case stall is bounded by
// MAX_CLIENTS * SEND_BUDGET_MS rather than by however long a peer feels like
// stalling.
constexpr int MAX_CLIENTS = 4;
constexpr int SEND_BUDGET_MS = 20;

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
	if (const char* env = std::getenv("WALLARENA_NET_ANY")) {
		m_bindAny = (env[0] != '\0' && env[0] != '0');
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
	// Loopback by default: this is a bridge to a local hardware driver, not a
	// service.  WALLARENA_NET_ANY=1 opens it to the rest of the network.
	addr.sin_addr.s_addr = htonl(m_bindAny ? INADDR_ANY : INADDR_LOOPBACK);
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

	std::printf("NetRenderer: broadcasting frames on %s:%u\n",
				m_bindAny ? "0.0.0.0" : "127.0.0.1", static_cast<unsigned>(m_port));
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
	while (true) {
		int fd = ::accept(m_listenFd, nullptr, nullptr);
		if (fd < 0) {
			if (errno == EINTR || errno == ECONNABORTED) continue; // retry
			break; // EAGAIN / EWOULDBLOCK: no more pending clients
		}

		if (static_cast<int>(m_clients.size()) >= MAX_CLIENTS) {
			// Refuse rather than let an unbounded set of peers each claim a
			// slice of the frame budget.
			std::printf("NetRenderer: refusing client (limit %d reached)\n", MAX_CLIENTS);
			::close(fd);
			continue;
		}

		int one = 1;
		::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
#ifdef SO_NOSIGPIPE
		::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
		// Non-blocking: SendAll polls with its own deadline, so a wedged peer
		// can never park the render thread inside send().
		::fcntl(fd, F_SETFL, O_NONBLOCK);

		m_clients.push_back(fd);
		std::printf("NetRenderer: client connected (%d total)\n",
					static_cast<int>(m_clients.size()));
	}
}

// Writes the whole message or gives up.  A partial write cannot be abandoned
// without corrupting the client's stream, so "gave up" always means "drop".
static bool SendAll(int fd, const char* p, size_t n) {
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(SEND_BUDGET_MS);

	while (n > 0) {
		ssize_t k = ::send(fd, p, n, MSG_NOSIGNAL);
		if (k > 0) {
			p += k;
			n -= static_cast<size_t>(k);
			continue;
		}
		if (k == 0) return false;
		if (errno == EINTR) continue;
		if (errno != EAGAIN && errno != EWOULDBLOCK) return false;

		auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
							 deadline - std::chrono::steady_clock::now())
							 .count();
		if (remaining <= 0) return false; // too slow

		pollfd pfd{};
		pfd.fd = fd;
		pfd.events = POLLOUT;
		int r = ::poll(&pfd, 1, static_cast<int>(remaining));
		if (r < 0 && errno == EINTR) continue;
		if (r <= 0) return false; // timed out or error
		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) return false;
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
	char buf[128];

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

// A NaN/inf coordinate would serialise as "nan"/"inf" and desync every client
// parsing the frame, so such points never enter a path.
static bool IsFinite(Vec2 v) {
	return std::isfinite(v.x) && std::isfinite(v.y);
}

void NetRenderer::DrawLine(Vec2 a, Vec2 b, Color c) {
	if (!IsFinite(a) || !IsFinite(b)) return;

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
	path.pts.reserve(pts.size() + 1);
	for (const Vec2& v : pts) {
		if (IsFinite(v)) path.pts.push_back(v);
	}
	if (path.pts.size() < 2) return;
	if (closed && path.pts.size() > 2) path.pts.push_back(path.pts.front());
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
