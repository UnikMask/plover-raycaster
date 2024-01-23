#include "plover_int.h"

// Global 
PloverContext ctx = {
    .renderer = {},
    .profilerRecording = false,
    .inputState = {
        .queue = {},
        .sendMouseMovements = true,
        .isMouseReset = true,
        .wasPrevMsgMouseMoved = false
    }
};

double getTime() {
	return glfwGetTime();
}

void mouseCallback(
	GLFWwindow* window,
	f64 position_x,
	f64 position_y) {
	if (ctx.inputState.sendMouseMovements) {
		// NOTE(oliver): Only have most recent mouse movement on queue
		if (ctx.inputState.queue.hasMessage() && 
            ctx.inputState.wasPrevMsgMouseMoved) {
			ctx.inputState.queue.pop();
		}
		InputMessage message{MOUSE_MOVED};
		message.v.mouseMoved.mousePosition = Vec2(position_x, position_y);
		message.v.mouseMoved.resetPrevious = ctx.inputState.isMouseReset;
		ctx.inputState.queue.push(message);

		ctx.inputState.isMouseReset = false;
		ctx.inputState.wasPrevMsgMouseMoved = true;
	}
}

void keyCallback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods) {
	InputMessage message{};
	switch (action) {
		case GLFW_PRESS: {
			message.tag = KEY_DOWN;
			message.v.keyDown.key = GLFWToKey(key);
			break;
		}
		case GLFW_RELEASE: {
			message.tag = KEY_UP;
			message.v.keyUp.key = GLFWToKey(key);
			break;
		}
	}

	ctx.inputState.wasPrevMsgMouseMoved = false;
	ctx.inputState.queue.push(message);

#ifndef NDEBUG
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		ctx.inputState.sendMouseMovements = false;
	}
	if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
		ctx.profilerRecording = true;
	}
	if (key == GLFW_KEY_F2 && action == GLFW_RELEASE) {
		ctx.profilerRecording = false;
	}
#endif
}

void clickCallback(
	GLFWwindow* window,
	int button,
	int action,
	int mods) {
#ifndef NDEBUG
	if (action == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		ctx.inputState.sendMouseMovements = true;
		ctx.inputState.isMouseReset = true;
	}
#endif
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		InputMessage message{};
		message.v.keyDown.key = MOUSE_LMB;
		if (action == GLFW_PRESS) {
			message.tag = KEY_DOWN;
		} else if (action == GLFW_RELEASE) {
			message.tag = KEY_UP;
		}
		ctx.inputState.wasPrevMsgMouseMoved = false;
		ctx.inputState.queue.push(message);
	}
}

internal_func bool isKeyDown(Key key) {
	return glfwGetKey(ctx.renderer.context->window, keyToGLFW(key)) == GLFW_PRESS;
}

InputMessage getInputMessage() {
	if (ctx.inputState.queue.hasMessage()) {
		return ctx.inputState.queue.pop();
	}
	return {NO_MESSAGE};
}

void pushRenderCommand(RenderCommand inMsg) {
	return ctx.renderer.commandQueue.push(inMsg);
}

bool hasRenderMessage() {
	return ctx.renderer.messageQueue.hasMessage();
}

RenderMessage popRenderMessage() {
	return ctx.renderer.messageQueue.pop();
}

void UI_Clear() {
	ctx.renderer.UI_Clear();
}

void UI_Rect(Vec4 color, UVec2 pos, UVec2 size) {
	ctx.renderer.UI_Rect(color, pos, size);
}

void UI_Text(Vec4 color, UVec2 pos, const char *format, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, format);
	vsprintf(buffer,format, args);
	ctx.renderer.UI_Text(color, pos, buffer);
	va_end (args);
}

void *arenaAllocate(MArena *arena, u64 size) {
	assert((u8*) arena->head + size <= (u8*) arena->base + arena->size && "Overflowed arena!");
	void *ptr = arena->head;
	arena->head = (u8*) arena->head + size;

	return ptr;
}
