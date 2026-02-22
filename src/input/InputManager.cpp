#include "input/InputManager.h"
#include <cstring>

InputManager::InputManager() {
	std::memset(m_prevFire, 0, sizeof(m_prevFire));
}

InputManager::~InputManager() {
	for (auto& slot : m_slots) {
		if (slot.controller) {
			SDL_GameControllerClose(slot.controller);
			slot.controller = nullptr;
		}
	}
}

void InputManager::Init() {
	AssignKeyboardSlots();

	// Open any already-connected controllers
	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
		if (SDL_IsGameController(i)) {
			HandleControllerAdded(i);
		}
	}
}

void InputManager::AssignKeyboardSlots() {
	// Slot 0: WASD + Space
	m_slots[0].type = InputSlot::DeviceType::KeyboardLeft;
	m_slots[0].active = true;
	// Slot 1: Arrow keys + RCtrl
	m_slots[1].type = InputSlot::DeviceType::KeyboardRight;
	m_slots[1].active = true;
}

void InputManager::HandleControllerAdded(int joystickIndex) {
	SDL_GameController* gc = SDL_GameControllerOpen(joystickIndex);
	if (!gc) return;

	SDL_JoystickID jid = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));

	// Find a free slot (starting after keyboard slots)
	for (int i = 2; i < MAX_PLAYERS; ++i) {
		if (!m_slots[i].active) {
			m_slots[i].type = InputSlot::DeviceType::Gamepad;
			m_slots[i].controller = gc;
			m_slots[i].joystickId = jid;
			m_slots[i].active = true;
			m_joinRequest = true;
			return;
		}
	}

	// No free slot
	SDL_GameControllerClose(gc);
}

void InputManager::HandleControllerRemoved(SDL_JoystickID id) {
	for (auto& slot : m_slots) {
		if (slot.type == InputSlot::DeviceType::Gamepad && slot.joystickId == id) {
			if (slot.controller) {
				SDL_GameControllerClose(slot.controller);
				slot.controller = nullptr;
			}
			slot.active = false;
			slot.type = InputSlot::DeviceType::None;
			slot.joystickId = -1;
			return;
		}
	}
}

void InputManager::Update(const SDL_Event& event) {
	if (event.type == SDL_CONTROLLERDEVICEADDED) {
		HandleControllerAdded(event.cdevice.which);
	} else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
		HandleControllerRemoved(event.cdevice.which);
	}
}

void InputManager::PollState() {
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		m_inputs[i].Reset();
	}
	PollKeyboard();
	PollGamepads();

	// Detect firePressed (rising edge)
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		m_inputs[i].firePressed = m_inputs[i].fire && !m_prevFire[i];
		m_prevFire[i] = m_inputs[i].fire;
	}
}

void InputManager::PollKeyboard() {
	const Uint8* keys = SDL_GetKeyboardState(nullptr);

	// Player 0: WASD + Space
	if (m_slots[0].active) {
		if (keys[SDL_SCANCODE_W]) m_inputs[0].moveForward += 1.0f;
		if (keys[SDL_SCANCODE_S]) m_inputs[0].moveForward -= 1.0f;
		if (keys[SDL_SCANCODE_A]) m_inputs[0].turnRight -= 1.0f;
		if (keys[SDL_SCANCODE_D]) m_inputs[0].turnRight += 1.0f;
		m_inputs[0].fire = keys[SDL_SCANCODE_SPACE] != 0;
	}

	// Player 1: Arrows + RCtrl
	if (m_slots[1].active) {
		if (keys[SDL_SCANCODE_UP])    m_inputs[1].moveForward += 1.0f;
		if (keys[SDL_SCANCODE_DOWN])  m_inputs[1].moveForward -= 1.0f;
		if (keys[SDL_SCANCODE_LEFT])  m_inputs[1].turnRight -= 1.0f;
		if (keys[SDL_SCANCODE_RIGHT]) m_inputs[1].turnRight += 1.0f;
		m_inputs[1].fire = keys[SDL_SCANCODE_RCTRL] != 0;
	}
}

void InputManager::PollGamepads() {
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (m_slots[i].type != InputSlot::DeviceType::Gamepad || !m_slots[i].active)
			continue;

		SDL_GameController* gc = m_slots[i].controller;
		if (!gc) continue;

		float lx = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f;
		float ly = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f;

		const float deadzone = 0.15f;
		if (std::abs(lx) < deadzone) lx = 0.0f;
		if (std::abs(ly) < deadzone) ly = 0.0f;

		m_inputs[i].turnRight = lx;
		m_inputs[i].moveForward = -ly; // SDL Y axis is inverted

		bool a = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A) != 0;
		bool rb = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) != 0;
		float rt = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0f;

		m_inputs[i].fire = a || rb || (rt > 0.5f);
	}
}

int InputManager::GetPlayerCount() const {
	int count = 0;
	for (const auto& slot : m_slots) {
		if (slot.active) ++count;
	}
	return count;
}

const PlayerInput& InputManager::GetPlayerInput(int playerIndex) const {
	static PlayerInput empty;
	if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return empty;
	return m_inputs[playerIndex];
}
