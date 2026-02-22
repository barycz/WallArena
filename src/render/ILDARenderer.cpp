#include "render/ILDARenderer.h"
#include <fstream>
#include <cmath>
#include <algorithm>

// ILDA Format 4 (Format 0 with RGB): 
// Each point is 8 bytes: x(2) y(2) status(1) color_b(1) color_g(1) color_r(1)
// Actually ILDA Format 5 (3D with true color) or Format 4 (2D with true color):
// We'll use Format 4 (2D Coordinate Data with True Color):
// Header: 32 bytes
// Points: 8 bytes each (x_hi, x_lo, y_hi, y_lo, status, b, g, r)

static constexpr int16_t ILDA_MIN = -32768;
static constexpr int16_t ILDA_MAX = 32767;

ILDARenderer::ILDARenderer() = default;
ILDARenderer::~ILDARenderer() { Shutdown(); }

bool ILDARenderer::Init(int windowWidth, int windowHeight) {
	(void)windowWidth;
	(void)windowHeight;
	m_frameCount = 0;
	m_allFrames.clear();
	return true;
}

void ILDARenderer::Shutdown() {
	// Write all accumulated frames to file on shutdown
	if (!m_allFrames.empty()) {
		std::ofstream file(m_outputPath, std::ios::binary);
		if (file.is_open()) {
			for (int i = 0; i < static_cast<int>(m_allFrames.size()); ++i) {
				const auto& frame = m_allFrames[i];
				WriteILDAFileHeader(file, i, static_cast<int>(frame.points.size()));

				for (const auto& pt : frame.points) {
					uint8_t xhi = static_cast<uint8_t>((pt.x >> 8) & 0xFF);
					uint8_t xlo = static_cast<uint8_t>(pt.x & 0xFF);
					uint8_t yhi = static_cast<uint8_t>((pt.y >> 8) & 0xFF);
					uint8_t ylo = static_cast<uint8_t>(pt.y & 0xFF);

					// Status byte: bit 6 = blanking, bit 7 = last point
					uint8_t status = 0;
					if (pt.blanked) status |= 0x40;

					file.put(static_cast<char>(xhi));
					file.put(static_cast<char>(xlo));
					file.put(static_cast<char>(yhi));
					file.put(static_cast<char>(ylo));
					file.put(static_cast<char>(status));
					file.put(static_cast<char>(pt.b));
					file.put(static_cast<char>(pt.g));
					file.put(static_cast<char>(pt.r));
				}
			}

			// Write closing header (empty frame)
			WriteILDAFileHeader(file, static_cast<int>(m_allFrames.size()), 0);
		}
	}
	m_allFrames.clear();
}

void ILDARenderer::Begin() {
	m_currentFrame.points.clear();
	m_hasLastPoint = false;
	m_lastPoint = {0, 0};
}

void ILDARenderer::End() {
	// Optionally set last point flag
	if (!m_currentFrame.points.empty()) {
		// No special handling needed; the ILDA last-point flag is in the header
	}

	m_allFrames.push_back(m_currentFrame);
	m_frameCount++;

	if (m_writeEveryFrame) {
		WriteFrameToFile();
	}
}

ILDAPoint ILDARenderer::WorldToILDA(Vec2 world, Color c, bool blanked) const {
	ILDAPoint pt;

	// Map world coords to ILDA range [-32768, 32767]
	// World: [0, worldWidth] -> [-32768, 32767]
	float nx = (world.x / m_worldWidth) * 2.0f - 1.0f;  // -1 to 1
	float ny = 1.0f - (world.y / m_worldHeight) * 2.0f;  // flip Y: ILDA Y up

	pt.x = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, nx * 32767.0f)));
	pt.y = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, ny * 32767.0f)));
	pt.r = c.r;
	pt.g = c.g;
	pt.b = c.b;
	pt.blanked = blanked;

	return pt;
}

void ILDARenderer::AddBlankingTravel(Vec2 target) {
	if (m_hasLastPoint) {
		// Add blanked point at current position, then blanked point at target
		// This creates a "blanking move" â€” the galvos travel with laser off
		m_currentFrame.points.push_back(WorldToILDA(m_lastPoint, Color::Black(), true));
		m_currentFrame.points.push_back(WorldToILDA(target, Color::Black(), true));
	}
}

void ILDARenderer::DrawLine(Vec2 a, Vec2 b, Color c) {
	// If we need to travel to 'a', add blanking
	if (!m_hasLastPoint || Vec2::Distance(m_lastPoint, a) > 1.0f) {
		AddBlankingTravel(a);
	}

	m_currentFrame.points.push_back(WorldToILDA(a, c, false));
	m_currentFrame.points.push_back(WorldToILDA(b, c, false));

	m_lastPoint = b;
	m_hasLastPoint = true;
}

void ILDARenderer::DrawPolyline(const std::vector<Vec2>& pts, Color c, bool closed) {
	if (pts.size() < 2) return;

	// Travel to first point
	if (!m_hasLastPoint || Vec2::Distance(m_lastPoint, pts[0]) > 1.0f) {
		AddBlankingTravel(pts[0]);
	}

	for (const auto& pt : pts) {
		m_currentFrame.points.push_back(WorldToILDA(pt, c, false));
	}

	if (closed && pts.size() > 2) {
		m_currentFrame.points.push_back(WorldToILDA(pts[0], c, false));
	}

	m_lastPoint = closed ? pts[0] : pts.back();
	m_hasLastPoint = true;
}

void ILDARenderer::DrawCircle(Vec2 center, float radius, Color c, int segments) {
	std::vector<Vec2> pts(segments);
	for (int i = 0; i < segments; ++i) {
		float angle = (2.0f * 3.14159265f * i) / segments;
		pts[i] = center + Vec2::FromAngle(angle) * radius;
	}
	DrawPolyline(pts, c, true);
}

void ILDARenderer::DrawRect(Vec2 topLeft, Vec2 size, Color c) {
	std::vector<Vec2> pts = {
		topLeft,
		{topLeft.x + size.x, topLeft.y},
		{topLeft.x + size.x, topLeft.y + size.y},
		{topLeft.x, topLeft.y + size.y}
	};
	DrawPolyline(pts, c, true);
}

void ILDARenderer::SetWorldBounds(float width, float height) {
	m_worldWidth = width;
	m_worldHeight = height;
}

void ILDARenderer::WriteFrameToFile() {
	std::ofstream file(m_outputPath, std::ios::binary | std::ios::app);
	if (!file.is_open()) return;

	const auto& frame = m_allFrames.back();
	WriteILDAFileHeader(file, m_frameCount - 1, static_cast<int>(frame.points.size()));

	for (const auto& pt : frame.points) {
		uint8_t xhi = static_cast<uint8_t>((pt.x >> 8) & 0xFF);
		uint8_t xlo = static_cast<uint8_t>(pt.x & 0xFF);
		uint8_t yhi = static_cast<uint8_t>((pt.y >> 8) & 0xFF);
		uint8_t ylo = static_cast<uint8_t>(pt.y & 0xFF);

		uint8_t status = 0;
		if (pt.blanked) status |= 0x40;

		file.put(static_cast<char>(xhi));
		file.put(static_cast<char>(xlo));
		file.put(static_cast<char>(yhi));
		file.put(static_cast<char>(ylo));
		file.put(static_cast<char>(status));
		file.put(static_cast<char>(pt.b));
		file.put(static_cast<char>(pt.g));
		file.put(static_cast<char>(pt.r));
	}
}

void ILDARenderer::WriteILDAFileHeader(std::ofstream& file, int frameIndex, int pointCount) {
	// ILDA Format 4 Header (32 bytes)
	// Bytes 0-3: "ILDA"
	file.write("ILDA", 4);

	// Bytes 4-6: Reserved (0)
	file.put(0); file.put(0); file.put(0);

	// Byte 7: Format code (4 = 2D with true color)
	file.put(4);

	// Bytes 8-15: Frame name (8 chars, padded with 0)
	const char frameName[] = "WallAren";
	file.write(frameName, 8);

	// Bytes 16-23: Company name (8 chars, padded with 0)
	const char compName[] = "WallAren";
	file.write(compName, 8);

	// Bytes 24-25: Number of points (big-endian 16-bit)
	uint16_t numPts = static_cast<uint16_t>(pointCount);
	file.put(static_cast<char>((numPts >> 8) & 0xFF));
	file.put(static_cast<char>(numPts & 0xFF));

	// Bytes 26-27: Frame number (big-endian 16-bit)
	uint16_t frameNum = static_cast<uint16_t>(frameIndex);
	file.put(static_cast<char>((frameNum >> 8) & 0xFF));
	file.put(static_cast<char>(frameNum & 0xFF));

	// Bytes 28-29: Total frames (big-endian 16-bit) â€” 0 means unknown
	file.put(0); file.put(0);

	// Byte 30: Scanner head (0)
	file.put(0);

	// Byte 31: Reserved (0)
	file.put(0);
}
