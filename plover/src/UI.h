#pragma once

#include <plover/plover.h>

#include "Texture.h"
#include "ttfRenderer.h"

#include <vma/vk_mem_alloc.h>

const u32 MAX_UI_QUADS = 4096;

struct VulkanContext;

struct UIQuad {
	u32  character;
	alignas(8) Vec2 pos;
	alignas(8) Vec2 size;
	alignas(16) Vec4 color;
};

struct UIContext {
	GlyphAtlas atlas;

	u32 quadsWritten;

	std::vector<void *> quadSSBOsMapped;
	std::vector<VkBuffer> quadSSBOBuffers;
	std::vector<VmaAllocation> quadSSBOAllocations;

	std::vector<VkDescriptorSet> descriptorSets;

    void createDescriptorSets(VulkanContext& context);

	void clear();
	void writeRect(VulkanContext *context,
				   Vec4 color,
				   UVec2 pos,
				   UVec2 size);
	void writeText(VulkanContext *context,
				   char *text,
				   Vec2 pos,
				   Vec4 color);
	void cleanup(VulkanContext *context);
};

void createUIPipeline(VulkanContext& context);

void createUIDescriptorSetLayout(VulkanContext& context);

void createUI(VulkanContext& context, UIContext *ui);
