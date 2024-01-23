#pragma once

#include <plover/plover.h>

#include "MessageQueue.h"
#include "VulkanContext.h"
#include "AssetLoader.h"

struct Renderer {
	VulkanContext* context;

	MessageQueue<RenderCommand> commandQueue;
	MessageQueue<RenderMessage> messageQueue;
	AssetLoader loader;

	void init();
	bool render();
	void cleanup();

	void processCommand(RenderCommand inCmd);
	void processCommands();

	void UI_Clear();
	void UI_Rect(Vec4 color, UVec2 pos, UVec2 size);
	void UI_Text(Vec4 color, UVec2 pos, char *text);
};

