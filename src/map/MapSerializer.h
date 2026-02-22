#pragma once
#include "map/Map.h"
#include <string>

class MapSerializer {
public:
	static bool Save(const Map& map, const std::string& filepath);
	static bool Load(Map& map, const std::string& filepath);
};
