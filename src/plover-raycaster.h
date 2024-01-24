
#include "plover/plover.h"

class EntityDatabase {
  public:
	Vec3 *position;
	int *spriteIndex;
};

class InputBinding {
	u64 mask;

  public:
	bool isHeld(u64 input) { return (mask & input) == mask; }

	InputBinding(u64 mask) { this->mask = mask; }
};

class InputBindings {
  public:
	InputBinding forward = InputBinding(KEY_W);
	InputBinding left = InputBinding(KEY_A);
	InputBinding backward = InputBinding(KEY_S);
	InputBinding right = InputBinding(KEY_D);
	InputBinding up = InputBinding(KEY_SHIFT + KEY_W);
	InputBinding down = InputBinding(KEY_SHIFT + KEY_S);
	InputBinding interact = InputBinding(MOUSE_LMB);
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
