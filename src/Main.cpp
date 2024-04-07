#include "glm/common.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"
#include "plover-raycaster.h"
#include "plover/plover.h"
#include <cstdint>
#include <iostream>

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
	state->bindings = DEFAULT_BINDINGS;
	state->mousePosition = Vec2(0, 0);
	state->previousMousePosition = Vec2(0, 0);
	state->camera.position = Vec3(3.5f, 15.5f, 3.5f);
	state->cameraPitch = -0.54f, state->cameraYaw = 0.87;
	state->buttonsPressed = 0;

	// Create needed materials
	handles.pushRenderCommand(
		{.tag = CREATE_MATERIAL,
		 .id = 0,
		 .v = {.createMaterial = {.textureName = "stones_color.png",
								  .normalName = "stones_nrm.png"}}});

	// Set up object positions
	state->loading = 10;
	state->db.position[0] = Vec3(3, 2, 3);
	state->db.position[1] = Vec3(3, 2, 6);
	state->db.position[2] = Vec3(3, 2, 9);
	state->db.position[3] = Vec3(6, 2, 3);
	state->db.position[4] = Vec3(6, 2, 6);
	state->db.position[5] = Vec3(6, 2, 9);
	state->db.position[6] = Vec3(9, 2, 3);
	state->db.position[7] = Vec3(9, 2, 6);
	state->db.position[8] = Vec3(9, 2, 9);
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
	mouseDelta *= 0.001;
	state->cameraYaw += mouseDelta.x;
	state->cameraPitch -= mouseDelta.y;
	state->cameraPitch = glm::clamp(state->cameraPitch, (f32)-HALF_PI + 0.01f,
									(f32)HALF_PI - 0.01f);
	state->previousMousePosition = state->mousePosition;

	// Update camera direction from yaw and pitch
	state->camera.direction =
		glm::normalize(Vec3(cos(state->cameraYaw) * cos(state->cameraPitch),
							sin(state->cameraPitch),
							sin(state->cameraYaw) * cos(state->cameraPitch)));

	// Player movement
	u64 playerInput = state->buttonsPressed;
	f32 speed = state->deltaTime;
	if (state->bindings.forward.isHeld(playerInput)) {
		state->camera.position += speed * state->camera.direction;
	}
	if (state->bindings.backward.isHeld(playerInput)) {
		state->camera.position -= speed * state->camera.direction;
	}
	if (state->bindings.right.isHeld(playerInput)) {
		state->camera.position +=
			speed *
			glm::normalize(glm::cross(state->camera.direction, Vec3(0, 1, 0)));
	}
	if (state->bindings.left.isHeld(playerInput)) {
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
	handles.UI_Text(color, UVec2(16, 26), "It's drawing! current FPS: %.2f",
					1.0 / state->deltaTime);

	// Draw frame
	handles.UI_Rect(color, UVec2(10, 10), UVec2(1260, 10));
	handles.UI_Rect(color, UVec2(10, 10), UVec2(10, 700));
	handles.UI_Rect(color, UVec2(1260, 10), UVec2(10, 700));
	handles.UI_Rect(color, UVec2(10, 700), UVec2(1260, 10));
}
internal_func void onReceivedInput(RenderMessage msg, GameState &game) {
	switch (msg.tag) {
	case MATERIAL_CREATED:
		game.materials[msg.cmdID] = msg.v.materialCreated.materialID;
		if (msg.cmdID == 0) {
			for (size_t i = 0; i < 9; i++) {
				handles.pushRenderCommand(
					{.tag = CREATE_MESH,
					 .id = (u32)i,
					 .v = {.createMesh = {
							   .name = "artefact.obj",
							   .materialID = msg.v.materialCreated.materialID,
						   }}});
			}
		}
		game.loading--;
		break;
	case MESH_CREATED:
		u32 i = msg.cmdID;
		game.db.spriteIndex[i] = msg.v.meshCreated.meshID;
		handles.pushRenderCommand(
			{.tag = SET_MESH_TRANSFORM,
			 .v = {
				 .setMeshTransform = {
					 .meshID = msg.v.meshCreated.meshID,
					 .transform = glm::translate(Mat4(1), game.db.position[i]) *
								  glm::rotate(Mat4(1), -glm::radians(90.0f),
											  Vec3(0, 0, 1))}}});
		game.loading--;
	}
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

	while (handles.hasRenderMessage()) {
		onReceivedInput(handles.popRenderMessage(), *state);
	}

	if (!state->loading) {
		state->deltaTime = handles.getTime() - state->prevFrameTime;
		handleInput(state);
		state->prevFrameTime = handles.getTime();
		handles.pushRenderCommand(
			{.tag = SET_CAMERA, .v = {.setCamera = {.camera = state->camera}}});

		handles.UI_Clear();

		drawUI(state);
	} else {
		std::cout << state->loading << std::endl;
	}
	return 0;
}
