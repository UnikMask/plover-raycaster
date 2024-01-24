#include "glm/common.hpp"
#include "glm/geometric.hpp"
#include "plover-raycaster.h"
#include "plover/plover.h"
#include <cstdint>

#define ENTITY_COUNT 16

static Handles *ploverHandles = nullptr;
static Handles handles{};

internal_func void *allocate(MArena &arena, u64 size) {
	assert((u8 *)arena.head + size <= (u8 *)arena.base + arena.size &&
		   "Overflowed arena!");
	void *ptr = arena.head;
	arena.head = (u8 *)arena.head + size;

	return ptr;
}

// Initialize game state and memory on startup
internal_func void init(GameMemory *mem) {
	GameState *state = (GameState *)mem->persistentArena.base;

	// Allocate entity database
	EntityDatabase *db = &state->db;
	db->position =
		(Vec3 *)allocate(mem->persistentArena, sizeof(Vec3) * ENTITY_COUNT);
	db->spriteIndex =
		(int *)allocate(mem->persistentArena, sizeof(int) * ENTITY_COUNT);

	// Setup game state
	state->deltaTime = handles.getTime();
	state->camera.position = Vec3(0.0f, 0.0f, -2.0f);
	state->cameraYaw = -HALF_PI;
	state->buttonsPressed = 0;
}

// Handle an input message. Returns whether there was a message.
internal_func bool handleInputMessage(InputMessage msg, GameState *state) {
	switch (msg.tag) {
	case KEY_DOWN: {
		if (msg.v.keyDown.key != KEY_NONE) {
			state->buttonsPressed |= (1 << static_cast<u64>(msg.v.keyDown.key));
		}
		return true;
	}
	case KEY_UP: {
		if (msg.v.keyUp.key != KEY_NONE) {
			state->buttonsPressed &=
				UINT64_MAX - (1 << static_cast<u64>(msg.v.keyUp.key));
		}
		return true;
	}
	case MOUSE_MOVED: {
		state->mousePosition = msg.v.mouseMoved.mousePosition;
		if (msg.v.mouseMoved.resetPrevious) {
			state->previousMousePosition = state->mousePosition;
		}
		return true;
	}
	default:
		return false;
	}
}

// Handle input for the current frame
internal_func void handleInput(GameState *state) {
	InputMessage msg = handles.getInputMessage();
	while (handleInputMessage(msg, state)) {
		msg = handles.getInputMessage();
	}

	// Mouse movement
	Vec2 mouseDelta = (state->mousePosition - state->previousMousePosition);
	state->cameraYaw -= mouseDelta.x;
	state->cameraPitch += mouseDelta.y;
	state->cameraPitch = glm::clamp(state->cameraPitch, (f32)-HALF_PI + 0.01f,
									(f32)HALF_PI - 0.01f);

	// Update camera direction from yaw and pitch
	state->camera.direction =
		glm::normalize(Vec3(cos(state->cameraYaw) * cos(state->cameraPitch),
							sin(state->cameraPitch),
							sin(state->cameraYaw) * cos(state->cameraPitch)));
	// Player movement
	u64 playerInput = state->buttonsPressed;
	f32 speed = state->deltaTime;
	if (state->bindings.forward.isHeld(playerInput)) {
		state->camera.position -= speed * state->camera.direction;
	}
	if (state->bindings.backward.isHeld(playerInput)) {
		state->camera.position += speed * state->camera.direction;
	}
	if (state->bindings.left.isHeld(playerInput)) {
		state->camera.position +=
			speed *
			glm::normalize(glm::cross(state->camera.direction, Vec3(0, 1, 0)));
	}
	if (state->bindings.right.isHeld(playerInput)) {
		state->camera.position -=
			speed *
			glm::normalize(glm::cross(state->camera.direction, Vec3(0, 1, 0)));
	}
	if (state->bindings.up.isHeld(playerInput)) {
		state->camera.position += speed * Vec3(0, 1, 0);
	}
	if (state->bindings.down.isHeld(playerInput)) {
		state->camera.position -= speed * Vec3(0, 1, 0);
	}
}

internal_func void drawUI(GameState *state) {
	Vec4 color = Vec4(0.914, 0.831, 0.612, 1.0);
	handles.UI_Text(color, UVec2(16, 26), "It's drawing! current time: %.4f",
					state->deltaTime);

	// Draw frame
	handles.UI_Rect(color, UVec2(10, 10), UVec2(1260, 10));
	handles.UI_Rect(color, UVec2(10, 10), UVec2(10, 700));
	handles.UI_Rect(color, UVec2(1260, 10), UVec2(10, 700));
	handles.UI_Rect(color, UVec2(10, 700), UVec2(1260, 10));
}

// Default loop on plover
EXPORT int gameUpdateAndRender(Handles *_pHandles, GameMemory *mem) {
	if (ploverHandles != _pHandles) {
		ploverHandles = _pHandles;
		handles = *_pHandles;
	}

	// Initialize game memory if that is not done
	if (!mem->initialized) {
		allocate(mem->persistentArena, sizeof(GameState));
		init(mem);
		mem->initialized = true;
	}

	// Game loop
	GameState *state = (GameState *)mem->persistentArena.base;
	state->deltaTime = handles.getTime() - state->prevFrameTime;
	state->prevFrameTime = handles.getTime();
	handles.UI_Clear();

	handleInput(state);
	drawUI(state);

	return 0;
}
