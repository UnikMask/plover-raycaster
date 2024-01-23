#pragma once

#include <plover/plover.h>

#include "MessageQueue.h"
#include "Renderer.h"

#include "glfw.h"

struct PloverContext {
    Renderer renderer;
    bool profilerRecording;
    struct {
        MessageQueue<InputMessage> queue;
        bool sendMouseMovements;
        bool isMouseReset;
        bool wasPrevMsgMouseMoved;
    } inputState;
};

struct linux_GameCode {
	long modificationTime;
	void *sharedObject;
	f_gameUpdateAndRender *updateAndRender;
};

extern PloverContext ctx;

void readFile(const char *path, u8 **buffer, u32 *bufferSize);
void DEBUG_log(const char *f, ...);

// Input callbacks
f64 getTime();
InputMessage getInputMessage();
void pushRenderCommand(RenderCommand inMsg);
bool hasRenderMessage();
RenderMessage popRenderMessage();
void mouseCallback(GLFWwindow* window, double position_x, double position_y);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void clickCallback(GLFWwindow* window, int button, int action, int mods);

void UI_Clear();
void UI_Rect(Vec4 color, UVec2 pos, UVec2 size);
void UI_Text(Vec4 color, UVec2 pos, const char *format, ...);

inline i32 keyToGLFW(Key key) {
	switch (key) {
		case KEY_W: return GLFW_KEY_W;
		case KEY_A: return GLFW_KEY_A;
		case KEY_S: return GLFW_KEY_S;
		case KEY_D: return GLFW_KEY_D;
		case KEY_SPACE: return GLFW_KEY_SPACE;
		case KEY_SHIFT: return GLFW_KEY_LEFT_SHIFT;
		case KEY_F1: return GLFW_KEY_F1;
		default: assert(false && "Key not supported!");
	};
	return 0;
}

inline Key GLFWToKey(i32 glfwKey) {
	switch (glfwKey) {
		case GLFW_KEY_W: return KEY_W;
		case GLFW_KEY_A: return KEY_A;
		case GLFW_KEY_S: return KEY_S;
		case GLFW_KEY_D: return KEY_D;
		case GLFW_KEY_SPACE: return KEY_SPACE;
		case GLFW_KEY_LEFT_SHIFT: return KEY_SHIFT;
		case GLFW_KEY_F1: return KEY_F1;
		default: return KEY_NONE;
	};
}
