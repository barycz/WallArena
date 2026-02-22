#pragma once
#include <cstdint>

struct Color {
	uint8_t r = 255;
	uint8_t g = 255;
	uint8_t b = 255;
	uint8_t a = 255;

	Color() = default;
	Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
		: r(r), g(g), b(b), a(a) {}

	static Color White()   { return {255, 255, 255}; }
	static Color Red()     { return {255, 0,   0};   }
	static Color Green()   { return {0,   255, 0};   }
	static Color Blue()    { return {0,   0,   255}; }
	static Color Yellow()  { return {255, 255, 0};   }
	static Color Cyan()    { return {0,   255, 255}; }
	static Color Magenta() { return {255, 0,   255}; }
	static Color Black()   { return {0,   0,   0};   }

	static Color FromIndex(int index) {
		const Color palette[] = {
			Red(), Blue(), Green(), Yellow(), Cyan(), Magenta(), White()
		};
		return palette[index % 7];
	}
};
