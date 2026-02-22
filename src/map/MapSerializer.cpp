#include "map/MapSerializer.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Simple text format:
// MAP <name>
// BOUNDS <width> <height>
// MODE <mode_int> <fragLimit> <timeLimit> <respawnLives>
// OBSTACLE <tag> <destructible> <r> <g> <b> <num_verts> <x1> <y1> <x2> <y2> ...
// SPAWN <x> <y> <angle>
// POWERUP <x> <y> <type_int>

bool MapSerializer::Save(const Map& map, const std::string& filepath) {
	std::ofstream file(filepath);
	if (!file.is_open()) return false;

	file << "MAP " << map.GetName() << "\n";
	file << "BOUNDS " << map.GetWidth() << " " << map.GetHeight() << "\n";

	const auto& mode = map.GetDefaultMode();
	file << "MODE " << static_cast<int>(mode.mode)
		 << " " << mode.fragLimit
		 << " " << mode.timeLimit
		 << " " << mode.respawnLives << "\n";

	for (const auto& obs : map.GetObstacles()) {
		file << "OBSTACLE "
			 << (obs.GetTag().empty() ? "_" : obs.GetTag()) << " "
			 << (obs.IsDestructible() ? 1 : 0) << " "
			 << static_cast<int>(obs.GetColor().r) << " "
			 << static_cast<int>(obs.GetColor().g) << " "
			 << static_cast<int>(obs.GetColor().b) << " "
			 << obs.GetVertices().size();
		for (const auto& v : obs.GetVertices()) {
			file << " " << v.x << " " << v.y;
		}
		file << "\n";
	}

	for (const auto& sp : map.GetSpawnPoints()) {
		file << "SPAWN " << sp.position.x << " " << sp.position.y
			 << " " << sp.angle << "\n";
	}

	for (const auto& ps : map.GetPowerUpSpawns()) {
		file << "POWERUP " << ps.position.x << " " << ps.position.y
			 << " " << static_cast<int>(ps.type) << "\n";
	}

	return true;
}

bool MapSerializer::Load(Map& map, const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) return false;

	map = Map(); // reset

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue;

		std::istringstream iss(line);
		std::string token;
		iss >> token;

		if (token == "MAP") {
			std::string name;
			std::getline(iss >> std::ws, name);
			map.SetName(name);
		}
		else if (token == "BOUNDS") {
			float w, h;
			iss >> w >> h;
			map.SetBounds(w, h);
		}
		else if (token == "MODE") {
			int modeInt, frag, lives;
			float timeLimit;
			iss >> modeInt >> frag >> timeLimit >> lives;
			auto& mode = map.GetDefaultMode();
			mode.mode = static_cast<GameMode>(modeInt);
			mode.fragLimit = frag;
			mode.timeLimit = timeLimit;
			mode.respawnLives = lives;
		}
		else if (token == "OBSTACLE") {
			std::string tag;
			int destructible, r, g, b;
			size_t numVerts;
			iss >> tag >> destructible >> r >> g >> b >> numVerts;

			std::vector<Vec2> verts(numVerts);
			for (size_t i = 0; i < numVerts; ++i) {
				iss >> verts[i].x >> verts[i].y;
			}

			Obstacle obs(verts, Color(
				static_cast<uint8_t>(r),
				static_cast<uint8_t>(g),
				static_cast<uint8_t>(b)));
			if (tag != "_") obs.SetTag(tag);
			obs.SetDestructible(destructible != 0);
			map.AddObstacle(obs);
		}
		else if (token == "SPAWN") {
			SpawnPoint sp;
			iss >> sp.position.x >> sp.position.y >> sp.angle;
			map.AddSpawnPoint(sp);
		}
		else if (token == "POWERUP") {
			PowerUpSpawnPoint ps;
			int typeInt;
			iss >> ps.position.x >> ps.position.y >> typeInt;
			ps.type = static_cast<PowerUpType>(typeInt);
			map.AddPowerUpSpawn(ps);
		}
	}

	return true;
}
