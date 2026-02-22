#pragma once
#include <vector>
#include <SDL.h>
#include "input/PlayerInput.h"

struct InputSlot {
	enum class DeviceType { None, KeyboardLeft, KeyboardRight, Gamepad };
	DeviceType type = DeviceType::None;
	SDL_GameController* controller = nullptr;
	SDL_JoystickID joystickId = -1;
	bool active = false;
};

class InputManager {
public:
	InputManager();
	~InputManager();

	void Init();
	void Update(const SDL_Event& event);
	void PollState();

	int GetPlayerCount() const;
	const PlayerInput& GetPlayerInput(int playerIndex) const;

	bool HasJoinRequest() const { return m_joinRequest; }
	void ClearJoinRequest() { m_joinRequest = false; }

	static constexpr int MAX_PLAYERS = 8;

private:
	void AssignKeyboardSlots();
	void HandleControllerAdded(int joystickIndex);
	void HandleControllerRemoved(SDL_JoystickID id);
	void PollKeyboard();
	void PollGamepads();

	InputSlot m_slots[MAX_PLAYERS];
	PlayerInput m_inputs[MAX_PLAYERS];
	bool m_prevFire[MAX_PLAYERS] = {};
	bool m_joinRequest = false;
};
