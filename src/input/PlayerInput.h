#pragma once

struct PlayerInput {
	float moveForward = 0.0f;   // -1.0 to 1.0 (back/forward)
	float turnRight = 0.0f;     // -1.0 to 1.0 (left/right)
	bool fire = false;
	bool firePressed = false;   // true only on the frame the button was pressed

	void Reset() {
		moveForward = 0.0f;
		turnRight = 0.0f;
		fire = false;
		firePressed = false;
	}
};
