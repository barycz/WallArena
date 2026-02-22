#include "render/VectorFont.h"

// Each character is defined as line segments on a 5x7 grid.
// Coordinates: (0,0) = top-left, (4,6) = bottom-right.

#define S(x1,y1,x2,y2) {x1,y1,x2,y2}

static const VectorFont::Segment seg_A[] = {
	S(0,6, 0,2), S(0,2, 2,0), S(2,0, 4,2), S(4,2, 4,6), S(0,4, 4,4)
};
static const VectorFont::Segment seg_B[] = {
	S(0,0, 0,6), S(0,0, 3,0), S(3,0, 4,1), S(4,1, 4,2), S(4,2, 3,3),
	S(3,3, 0,3), S(3,3, 4,4), S(4,4, 4,5), S(4,5, 3,6), S(3,6, 0,6)
};
static const VectorFont::Segment seg_C[] = {
	S(4,1, 3,0), S(3,0, 1,0), S(1,0, 0,1), S(0,1, 0,5), S(0,5, 1,6), S(1,6, 3,6), S(3,6, 4,5)
};
static const VectorFont::Segment seg_D[] = {
	S(0,0, 0,6), S(0,0, 3,0), S(3,0, 4,1), S(4,1, 4,5), S(4,5, 3,6), S(3,6, 0,6)
};
static const VectorFont::Segment seg_E[] = {
	S(0,0, 0,6), S(0,0, 4,0), S(0,3, 3,3), S(0,6, 4,6)
};
static const VectorFont::Segment seg_F[] = {
	S(0,0, 0,6), S(0,0, 4,0), S(0,3, 3,3)
};
static const VectorFont::Segment seg_G[] = {
	S(4,1, 3,0), S(3,0, 1,0), S(1,0, 0,1), S(0,1, 0,5), S(0,5, 1,6), S(1,6, 3,6),
	S(3,6, 4,5), S(4,5, 4,3), S(4,3, 2,3)
};
static const VectorFont::Segment seg_H[] = {
	S(0,0, 0,6), S(4,0, 4,6), S(0,3, 4,3)
};
static const VectorFont::Segment seg_I[] = {
	S(1,0, 3,0), S(2,0, 2,6), S(1,6, 3,6)
};
static const VectorFont::Segment seg_J[] = {
	S(1,0, 4,0), S(3,0, 3,5), S(3,5, 2,6), S(2,6, 1,6), S(1,6, 0,5)
};
static const VectorFont::Segment seg_K[] = {
	S(0,0, 0,6), S(4,0, 0,3), S(0,3, 4,6)
};
static const VectorFont::Segment seg_L[] = {
	S(0,0, 0,6), S(0,6, 4,6)
};
static const VectorFont::Segment seg_M[] = {
	S(0,6, 0,0), S(0,0, 2,3), S(2,3, 4,0), S(4,0, 4,6)
};
static const VectorFont::Segment seg_N[] = {
	S(0,6, 0,0), S(0,0, 4,6), S(4,6, 4,0)
};
static const VectorFont::Segment seg_O[] = {
	S(1,0, 3,0), S(3,0, 4,1), S(4,1, 4,5), S(4,5, 3,6), S(3,6, 1,6),
	S(1,6, 0,5), S(0,5, 0,1), S(0,1, 1,0)
};
static const VectorFont::Segment seg_P[] = {
	S(0,0, 0,6), S(0,0, 3,0), S(3,0, 4,1), S(4,1, 4,2), S(4,2, 3,3), S(3,3, 0,3)
};
static const VectorFont::Segment seg_Q[] = {
	S(1,0, 3,0), S(3,0, 4,1), S(4,1, 4,5), S(4,5, 3,6), S(3,6, 1,6),
	S(1,6, 0,5), S(0,5, 0,1), S(0,1, 1,0), S(3,5, 4,6)
};
static const VectorFont::Segment seg_R[] = {
	S(0,0, 0,6), S(0,0, 3,0), S(3,0, 4,1), S(4,1, 4,2), S(4,2, 3,3), S(3,3, 0,3), S(2,3, 4,6)
};
static const VectorFont::Segment seg_S[] = {
	S(4,1, 3,0), S(3,0, 1,0), S(1,0, 0,1), S(0,1, 0,2), S(0,2, 1,3),
	S(1,3, 3,3), S(3,3, 4,4), S(4,4, 4,5), S(4,5, 3,6), S(3,6, 1,6), S(1,6, 0,5)
};
static const VectorFont::Segment seg_T[] = {
	S(0,0, 4,0), S(2,0, 2,6)
};
static const VectorFont::Segment seg_U[] = {
	S(0,0, 0,5), S(0,5, 1,6), S(1,6, 3,6), S(3,6, 4,5), S(4,5, 4,0)
};
static const VectorFont::Segment seg_V[] = {
	S(0,0, 2,6), S(2,6, 4,0)
};
static const VectorFont::Segment seg_W[] = {
	S(0,0, 0,6), S(0,6, 2,4), S(2,4, 4,6), S(4,6, 4,0)
};
static const VectorFont::Segment seg_X[] = {
	S(0,0, 4,6), S(4,0, 0,6)
};
static const VectorFont::Segment seg_Y[] = {
	S(0,0, 2,3), S(4,0, 2,3), S(2,3, 2,6)
};
static const VectorFont::Segment seg_Z[] = {
	S(0,0, 4,0), S(4,0, 0,6), S(0,6, 4,6)
};

// Numbers
static const VectorFont::Segment seg_0[] = {
	S(1,0, 3,0), S(3,0, 4,1), S(4,1, 4,5), S(4,5, 3,6), S(3,6, 1,6),
	S(1,6, 0,5), S(0,5, 0,1), S(0,1, 1,0), S(0,5, 4,1)
};
static const VectorFont::Segment seg_1[] = {
	S(1,1, 2,0), S(2,0, 2,6), S(1,6, 3,6)
};
static const VectorFont::Segment seg_2[] = {
	S(0,1, 1,0), S(1,0, 3,0), S(3,0, 4,1), S(4,1, 4,2), S(4,2, 0,6), S(0,6, 4,6)
};
static const VectorFont::Segment seg_3[] = {
	S(0,1, 1,0), S(1,0, 3,0), S(3,0, 4,1), S(4,1, 4,2), S(4,2, 3,3),
	S(3,3, 2,3), S(3,3, 4,4), S(4,4, 4,5), S(4,5, 3,6), S(3,6, 1,6), S(1,6, 0,5)
};
static const VectorFont::Segment seg_4[] = {
	S(0,0, 0,3), S(0,3, 4,3), S(4,0, 4,6)
};
static const VectorFont::Segment seg_5[] = {
	S(4,0, 0,0), S(0,0, 0,3), S(0,3, 3,3), S(3,3, 4,4), S(4,4, 4,5), S(4,5, 3,6), S(3,6, 1,6), S(1,6, 0,5)
};
static const VectorFont::Segment seg_6[] = {
	S(3,0, 1,0), S(1,0, 0,1), S(0,1, 0,5), S(0,5, 1,6), S(1,6, 3,6),
	S(3,6, 4,5), S(4,5, 4,4), S(4,4, 3,3), S(3,3, 0,3)
};
static const VectorFont::Segment seg_7[] = {
	S(0,0, 4,0), S(4,0, 2,6)
};
static const VectorFont::Segment seg_8[] = {
	S(1,0, 3,0), S(3,0, 4,1), S(4,1, 4,2), S(4,2, 3,3), S(3,3, 1,3),
	S(1,3, 0,2), S(0,2, 0,1), S(0,1, 1,0),
	S(1,3, 0,4), S(0,4, 0,5), S(0,5, 1,6), S(1,6, 3,6), S(3,6, 4,5), S(4,5, 4,4), S(4,4, 3,3)
};
static const VectorFont::Segment seg_9[] = {
	S(4,3, 1,3), S(1,3, 0,2), S(0,2, 0,1), S(0,1, 1,0), S(1,0, 3,0),
	S(3,0, 4,1), S(4,1, 4,5), S(4,5, 3,6), S(3,6, 1,6)
};

// Punctuation
static const VectorFont::Segment seg_DASH[] = {
	S(1,3, 3,3)
};
static const VectorFont::Segment seg_COLON[] = {
	S(2,2, 2,2), S(2,4, 2,4)
};
static const VectorFont::Segment seg_DOT[] = {
	S(2,6, 2,6)
};
static const VectorFont::Segment seg_SLASH[] = {
	S(4,0, 0,6)
};
static const VectorFont::Segment seg_LPAREN[] = {
	S(3,0, 1,2), S(1,2, 1,4), S(1,4, 3,6)
};
static const VectorFont::Segment seg_RPAREN[] = {
	S(1,0, 3,2), S(3,2, 3,4), S(3,4, 1,6)
};
static const VectorFont::Segment seg_EXCLAM[] = {
	S(2,0, 2,4), S(2,5.5f, 2,6)
};

#undef S

#define CHAR_CASE(ch, arr) case ch: segments = arr; count = sizeof(arr)/sizeof(arr[0]); return segments

const VectorFont::Segment* VectorFont::GetCharSegments(char c, int& count) {
	const Segment* segments = nullptr;
	count = 0;

	// Convert to uppercase
	if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';

	switch (c) {
		CHAR_CASE('A', seg_A);
		CHAR_CASE('B', seg_B);
		CHAR_CASE('C', seg_C);
		CHAR_CASE('D', seg_D);
		CHAR_CASE('E', seg_E);
		CHAR_CASE('F', seg_F);
		CHAR_CASE('G', seg_G);
		CHAR_CASE('H', seg_H);
		CHAR_CASE('I', seg_I);
		CHAR_CASE('J', seg_J);
		CHAR_CASE('K', seg_K);
		CHAR_CASE('L', seg_L);
		CHAR_CASE('M', seg_M);
		CHAR_CASE('N', seg_N);
		CHAR_CASE('O', seg_O);
		CHAR_CASE('P', seg_P);
		CHAR_CASE('Q', seg_Q);
		CHAR_CASE('R', seg_R);
		CHAR_CASE('S', seg_S);
		CHAR_CASE('T', seg_T);
		CHAR_CASE('U', seg_U);
		CHAR_CASE('V', seg_V);
		CHAR_CASE('W', seg_W);
		CHAR_CASE('X', seg_X);
		CHAR_CASE('Y', seg_Y);
		CHAR_CASE('Z', seg_Z);
		CHAR_CASE('0', seg_0);
		CHAR_CASE('1', seg_1);
		CHAR_CASE('2', seg_2);
		CHAR_CASE('3', seg_3);
		CHAR_CASE('4', seg_4);
		CHAR_CASE('5', seg_5);
		CHAR_CASE('6', seg_6);
		CHAR_CASE('7', seg_7);
		CHAR_CASE('8', seg_8);
		CHAR_CASE('9', seg_9);
		CHAR_CASE('-', seg_DASH);
		CHAR_CASE(':', seg_COLON);
		CHAR_CASE('.', seg_DOT);
		CHAR_CASE('/', seg_SLASH);
		CHAR_CASE('(', seg_LPAREN);
		CHAR_CASE(')', seg_RPAREN);
		CHAR_CASE('!', seg_EXCLAM);
		default:
			count = 0;
			return nullptr;
	}
}

#undef CHAR_CASE

void VectorFont::DrawText(IRenderer& renderer, const std::string& text,
						   Vec2 position, float scale, Color color) {
	float cx = position.x;
	for (char c : text) {
		if (c == ' ') {
			cx += (CHAR_WIDTH + CHAR_SPACING) * scale;
			continue;
		}

		int count = 0;
		const Segment* segs = GetCharSegments(c, count);
		if (segs) {
			for (int i = 0; i < count; ++i) {
				Vec2 a = {cx + segs[i].x1 * scale, position.y + segs[i].y1 * scale};
				Vec2 b = {cx + segs[i].x2 * scale, position.y + segs[i].y2 * scale};
				renderer.DrawLine(a, b, color);
			}
		}
		cx += (CHAR_WIDTH + CHAR_SPACING) * scale;
	}
}

float VectorFont::MeasureWidth(const std::string& text, float scale) {
	if (text.empty()) return 0.0f;
	return static_cast<float>(text.size()) * (CHAR_WIDTH + CHAR_SPACING) * scale - CHAR_SPACING * scale;
}

void VectorFont::DrawTextCentered(IRenderer& renderer, const std::string& text,
								   float centerX, float y, float scale, Color color) {
	float w = MeasureWidth(text, scale);
	DrawText(renderer, text, {centerX - w * 0.5f, y}, scale, color);
}
