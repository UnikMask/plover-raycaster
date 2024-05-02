#include "UI.h"
#include "VulkanContext.h"

void createUIDescriptorSetLayout(VulkanContext &context) {
	VkDescriptorSetLayoutBinding layoutBindings[2];
	layoutBindings[0] = {};
	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorType =
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].pImmutableSamplers = nullptr;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	layoutBindings[1] = {};
	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = layoutBindings;

	if (vkCreateDescriptorSetLayout(context.device, &layoutInfo, nullptr,
									&context.uiDescriptorSetLayout) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to create ui descriptor set layout!");
	}
}

void UIContext::createDescriptorSets(VulkanContext &context) {
	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	quadSSBOsMapped.resize(MAX_FRAMES_IN_FLIGHT);
	quadSSBOBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	quadSSBOAllocations.resize(MAX_FRAMES_IN_FLIGHT);

	context.descriptorAllocator.allocate(MAX_FRAMES_IN_FLIGHT,
										 descriptorSets.data(),
										 context.uiDescriptorSetLayout);

	// Create SSBOs
	VkDeviceSize quadSSBOSize = sizeof(UIQuad) * MAX_UI_QUADS;
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		CreateBufferInfo quadSSBOInfo{};
		quadSSBOInfo.size = quadSSBOSize;
		quadSSBOInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		quadSSBOInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
								  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		quadSSBOInfo.vmaFlags = static_cast<VmaAllocationCreateFlagBits>(
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT);
		context.createBuffer(quadSSBOInfo, quadSSBOBuffers[i],
							 quadSSBOAllocations[i]);

		VmaAllocationInfo quadSSBOAllocInfo{};
		vmaGetAllocationInfo(context.allocator, quadSSBOAllocations[i],
							 &quadSSBOAllocInfo);
		quadSSBOsMapped[i] = quadSSBOAllocInfo.pMappedData;
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = atlas.texture.imageView;
		imageInfo.sampler = atlas.texture.sampler;

		VkDescriptorBufferInfo ssboInfo{};
		ssboInfo.buffer = quadSSBOBuffers[i];
		ssboInfo.offset = 0;
		ssboInfo.range = quadSSBOSize;

		VkWriteDescriptorSet descriptorWrites[2];
		descriptorWrites[0] = {};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &imageInfo;

		descriptorWrites[1] = {};
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &ssboInfo;

		vkUpdateDescriptorSets(context.device, 2, descriptorWrites, 0, nullptr);
	}
}

void createUIPipeline(VulkanContext &context) {
	PipelineCreateInfo createInfo{};
	createInfo.useDepthBuffer = false;
	createInfo.doCulling = true;
	createInfo.subpass = 1; // UI Subpass
	createInfo.vertexShaderPath = "../resources/spirv/ui.vert.spv";
	createInfo.fragmentShaderPath = "../resources/spirv/ui.frag.spv";
	createInfo.descriptorSetLayoutCount = 1;
	createInfo.pDescriptorSetLayouts = &context.uiDescriptorSetLayout;
	createInfo.bindingDescriptionCount = 0;
	createInfo.pBindingDescriptions = nullptr;
	createInfo.attributeDescriptionCount = 0;
	createInfo.pAttributeDescriptions = nullptr;

	context.createGraphicsPipeline(createInfo, context.uiPipeline,
								   context.uiPipelineLayout);
}

void createUI(VulkanContext &context, UIContext *ui) {
	buildGlyphAtlas(&context, &ui->atlas);

	ui->createDescriptorSets(context);
}

void UIContext::clear() { quadsWritten = 0; }

void UIContext::writeRect(VulkanContext *context, Vec4 color, UVec2 pos,
						  UVec2 size) {
	assert(quadsWritten < MAX_UI_QUADS);

	UIQuad quad{};
	quad.character = 255;
	quad.pos = Vec2(pos.x / (f32)WIDTH, pos.y / (f32)HEIGHT) * 2.0f - 1.0f;
	quad.size = Vec2(size.x / (f32)WIDTH, size.y / (f32)HEIGHT) * 2.0f;
	quad.color = color;

	memcpy((UIQuad *)quadSSBOsMapped[context->currentFrame] + quadsWritten,
		   &quad, sizeof(UIQuad));

	quadsWritten++;
}

void UIContext::writeText(VulkanContext *context, char *text, Vec2 pos,
						  Vec4 color) {
	assert(quadsWritten < MAX_UI_QUADS);

	CharData charData;
	u32 idx = 0;
	u32 xOffset = 0;
	i32 kerning_x = 0;

	while (text[idx] != '\0') {
		char ch = text[idx];
		char nextCh = text[idx + 1];
		u8 charIdx = text[idx] - MIN_PRINTABLE_CHAR;
		charData = atlas.chars[charIdx];

		kerning_x = 0;
		for (i32 i = 0; i < charData.kernData.size(); i++) {
			KernData kData = charData.kernData[i];
			if (kData.character == nextCh) {
				kerning_x = kData.kerning_x;
			}
		}

		UIQuad quad{};
		quad.character = charIdx;
		quad.pos = Vec2((xOffset + pos.x - kerning_x) / (f32)WIDTH,
						pos.y / (f32)HEIGHT) *
					   2.0f -
				   1.0f;
		quad.size = Vec2(atlas.charQuadWidth / (f32)WIDTH,
						 atlas.charQuadHeight / (f32)HEIGHT) *
					2.0f;
		quad.color = color;

		memcpy((UIQuad *)quadSSBOsMapped[context->currentFrame] + quadsWritten,
			   &quad, sizeof(UIQuad));

		xOffset += charData.xAdvance;
		idx++;
		quadsWritten++;
	}
}

void UIContext::cleanup(VulkanContext *context) {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vmaDestroyBuffer(context->allocator, quadSSBOBuffers[i],
						 quadSSBOAllocations[i]);
	}

	vkDestroySampler(context->device, atlas.texture.sampler, nullptr);
	vkDestroyImageView(context->device, atlas.texture.imageView, nullptr);
	vmaDestroyImage(context->allocator, atlas.texture.image,
					atlas.texture.allocation);
}
