
#include "plover/plover.h"

class EntityDatabase {
  public:
	Vec3 *position;
	int *spriteIndex;
};

class InputBinding {
  public:
	u64 mask;

	bool isHeld(u64 input) { return (mask & input) == mask; }
};

class InputBindings {
  public:
	InputBinding forward;
	InputBinding left;
	InputBinding backward;
	InputBinding right;
	InputBinding up;
	InputBinding down;
	InputBinding interact;
};

const static InputBindings DEFAULT_BINDINGS = InputBindings{
	.forward = {.mask = 1 << KEY_W},
	.left = {.mask = 1 << KEY_A},
	.backward = {.mask = 1 << KEY_S},
	.right = {.mask = 1 << KEY_D},
	.up = {.mask = 1 << KEY_SPACE},
	.down = {.mask = 1 << KEY_SHIFT},
	.interact = {.mask = 1 << MOUSE_LMB},
};

class GameState {
  public:
	// Camera
	Camera camera;
	f32 cameraYaw;
	f32 cameraPitch;

	// Frame time
	f64 prevFrameTime;
	f64 deltaTime;

	// Input
	u64 buttonsPressed;
	InputBindings bindings;
	Vec2 mousePosition;
	Vec2 previousMousePosition;
	Vec2 pointerPosition;

	// Management
	EntityDatabase db;
};
