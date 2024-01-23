enum Key {
	KEY_NONE,
	KEY_W,
	KEY_A,
	KEY_S,
	KEY_D,
	KEY_SPACE,
	KEY_SHIFT,
	KEY_F1,
	MOUSE_LMB
};

typedef glm::vec2 MouseDelta;

enum InputMessageTag {
	NO_MESSAGE,
	KEY_DOWN,
	KEY_UP,
	MOUSE_MOVED,
};

struct KeyData {
	Key key;
};

struct MouseData {
	Vec2 mousePosition;
	bool resetPrevious;
};

struct InputMessage {
	InputMessageTag tag;
	union {
		KeyData keyDown;
		KeyData keyUp;
		MouseData mouseMoved;
	} v;
};
