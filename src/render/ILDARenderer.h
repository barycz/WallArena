#pragma once
#include "render/IRenderer.h"
#include <vector>
#include <string>
#include <cstdint>

// ILDA point: 16-bit coordinates, RGB color, blanking flag
struct ILDAPoint {
	int16_t x = 0;
	int16_t y = 0;
	uint8_t r = 255;
	uint8_t g = 255;
	uint8_t b = 255;
	bool blanked = false;
};

// ILDA frame: a list of points
struct ILDAFrame {
	std::vector<ILDAPoint> points;
};

class ILDARenderer : public IRenderer {
public:
	ILDARenderer();
	~ILDARenderer() override;

	bool Init(int windowWidth, int windowHeight) override;
	void Shutdown() override;

	void Begin() override;
	void End() override;

	void DrawLine(Vec2 a, Vec2 b, Color c) override;
	void DrawPolyline(const std::vector<Vec2>& pts, Color c, bool closed = false) override;
	void DrawCircle(Vec2 center, float radius, Color c, int segments = 16) override;
	void DrawRect(Vec2 topLeft, Vec2 size, Color c) override;

	void SetWorldBounds(float width, float height) override;

	// ILDA-specific
	void SetOutputPath(const std::string& path) { m_outputPath = path; }
	const ILDAFrame& GetCurrentFrame() const { return m_currentFrame; }
	int GetFrameCount() const { return m_frameCount; }

private:
	ILDAPoint WorldToILDA(Vec2 world, Color c, bool blanked = false) const;
	void AddBlankingTravel(Vec2 target);
	void WriteFrameToFile();
	void WriteILDAFileHeader(std::ofstream& file, int frameIndex, int pointCount);

	float m_worldWidth = 1000.0f;
	float m_worldHeight = 1000.0f;

	ILDAFrame m_currentFrame;
	std::vector<ILDAFrame> m_allFrames;
	Vec2 m_lastPoint = {0, 0};
	bool m_hasLastPoint = false;

	std::string m_outputPath = "output.ild";
	int m_frameCount = 0;
	bool m_writeEveryFrame = false; // if true, appends each frame to file
};
