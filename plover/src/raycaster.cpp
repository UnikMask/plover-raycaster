#include "raycaster.h"
#include "Mesh.h"
#include "Texture.h"
#include "VulkanContext.h"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "plover/plover.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

RaycasterContext::RaycasterContext(Level level, VulkanContext *context) {
	this->context = context;
	this->level = level;
	createDescriptorSetLayout();
	createUniformBuffers();
	createLevelExtrasBuf();
	uploadToExtrasUniform();
	createVertexBuffer();
	createDescriptorSets();
	createRaycasterPipeline();
}

RaycasterContext::~RaycasterContext() {
	vkDestroyDescriptorSetLayout(context->device, descriptorSetLayout, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vmaDestroyBuffer(context->allocator, uniformBuffers[i],
						 uniformBufferAllocations[i]);
	}
	vmaDestroyBuffer(context->allocator, vertexBuffer, vertexBufferAlloc);
	vkDestroyPipeline(context->device, pipeline, nullptr);
	vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
	context = nullptr;
}

void RaycasterContext::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(RaycasterUniform);

	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	// Create uniform buffer and its info.
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = bufferSize,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE};

		VmaAllocationCreateInfo allocationCreateInfo{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
					 VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO,
			.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

		VmaAllocationInfo allocInfo{};
		vmaCreateBuffer(context->allocator, &bufferInfo, &allocationCreateInfo,
						&uniformBuffers[i], &uniformBufferAllocations[i],
						&allocInfo);
		uniformBuffersMapped[i] = allocInfo.pMappedData;
	}
}

void RaycasterContext::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = nullptr};

	VkDescriptorSetLayoutBinding levelBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = 0};

	VkDescriptorSetLayoutBinding extrasBinding{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr};

	std::vector<VkDescriptorSetLayoutBinding> bindings = {
		uboBinding, levelBinding, extrasBinding};

	VkDescriptorSetLayoutCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = (uint32_t)bindings.size(),
		.pBindings = bindings.data()};
	if (vkCreateDescriptorSetLayout(context->device, &info, nullptr,
									&descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error(
			"Failed to create raycaster descriptor set layout!");
	}
}

void RaycasterContext::createVertexBuffer() {
	VkDeviceSize size = raycasterVertices.size() * sizeof(RaycasterVertex);

	CreateBufferInfo vertexCreateInfo{
		.size = size,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.vmaFlags = static_cast<VmaAllocationCreateFlagBits>(0)};
	context->createBuffer(vertexCreateInfo, vertexBuffer, vertexBufferAlloc);

	VkBuffer stagingBuf;
	VmaAllocation stagingBufAlloc;
	CreateBufferInfo stagingBufInfo{
		.size = size,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		.vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT};

	context->createBuffer(stagingBufInfo, stagingBuf, stagingBufAlloc);

	void *data;
	vmaMapMemory(context->allocator, stagingBufAlloc, &data);
	memcpy(data, raycasterVertices.data(), size);
	vmaUnmapMemory(context->allocator, stagingBufAlloc);

	context->copyBuffer(stagingBuf, vertexBuffer, size);
	vmaDestroyBuffer(context->allocator, stagingBuf, stagingBufAlloc);
}

void RaycasterContext::createLevelExtrasBuf() {
	CreateBufferInfo extrasCreateInfo{
		.size = sizeof(RaycasterExtrasUniform),
		.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.vmaFlags = 0};
	context->createBuffer(extrasCreateInfo, levelExtrasBuf, levelExtrasAlloc);
}

void RaycasterContext::uploadToExtrasUniform() {
	VkBuffer stagingBuf;
	VmaAllocation stagingAlloc;
	CreateBufferInfo stagingInfo{
		.size = sizeof(RaycasterExtrasUniform),
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		.vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT};
	context->createBuffer(stagingInfo, stagingBuf, stagingAlloc);
	RaycasterExtrasUniform u = {.extent = level.extent};
	for (size_t i = 0; i < 256; i++) {
		u.palette[i] = level.palette[i];
	}

	void *data;
	vmaMapMemory(context->allocator, stagingAlloc, &data);
	memcpy(data, &u, sizeof(RaycasterExtrasUniform));
	vmaUnmapMemory(context->allocator, stagingAlloc);
	context->copyBuffer(stagingBuf, levelExtrasBuf,
						sizeof(RaycasterExtrasUniform));
	vmaDestroyBuffer(context->allocator, stagingBuf, stagingAlloc);
}

void RaycasterContext::createDescriptorSets() {
	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	context->descriptorAllocator.allocate(
		MAX_FRAMES_IN_FLIGHT, descriptorSets.data(), descriptorSetLayout);

	VkDescriptorBufferInfo levelBuf{
		.buffer = level.buf, .offset = 0, .range = level.size()};
	VkDescriptorBufferInfo extrasBufferInfo{.buffer = levelExtrasBuf,
											.offset = 0,
											.range =
												sizeof(RaycasterExtrasUniform)};
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{.buffer = uniformBuffers[i],
										  .offset = 0,
										  .range = sizeof(RaycasterUniform)};

		// Camera uniform buffer information
		VkWriteDescriptorSet uboWrite{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &bufferInfo};

		VkWriteDescriptorSet levelWrite{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = descriptorSets[i],
			.dstBinding = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &levelBuf};

		// Level extra information
		VkWriteDescriptorSet levelExtrasWrite{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = descriptorSets[i],
			.dstBinding = 2,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &extrasBufferInfo};

		std::vector<VkWriteDescriptorSet> infoArray = {uboWrite, levelWrite,
													   levelExtrasWrite};
		vkUpdateDescriptorSets(context->device, (size_t)infoArray.size(),
							   infoArray.data(), 0, nullptr);
	}
}

void RaycasterContext::createRaycasterPipeline() {
	VkDescriptorSetLayout layouts[1] = {descriptorSetLayout};

	auto attributeDescriptions = RaycasterVertex::getAttributeDescriptions();
	auto bindingDescription = RaycasterVertex::getBindingDescription();

	PipelineCreateInfo createInfo{
		.useDepthBuffer = true,
		.doCulling = true,
		.wireframeMode = false,
		.subpass = 0,
		.vertexShaderPath = "../resources/spirv/raycaster.vert.spv",
		.fragmentShaderPath = "../resources/spirv/raycaster.frag.spv",
		.descriptorSetLayoutCount = 1,
		.pDescriptorSetLayouts = &descriptorSetLayout,
		.bindingDescriptionCount = 1,
		.pBindingDescriptions = &bindingDescription,
		.attributeDescriptionCount = (uint32_t)attributeDescriptions.size(),
		.pAttributeDescriptions = attributeDescriptions.data()};

	context->createGraphicsPipeline(createInfo, pipeline, pipelineLayout);
}

void RaycasterContext::updateUniform(uint32_t currentImage) {
	RaycasterUniform ro{.cameraPos = context->camera.position,
						.cameraDir = context->camera.direction,
						.cameraUp = glm::vec3(0.0f, 1.0f, 0.0f),
						.fov = glm::radians(45.0f),
						.aspectRatio = context->swapChainExtent.width /
									   (float)context->swapChainExtent.height,
						.minDistance = 0.1f,
						.maxDistance = 100.0f};
	ro.cameraLeft =
		glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), ro.cameraDir));
	ro.cameraUp = glm::cross(ro.cameraLeft, ro.cameraDir);
	memcpy(uniformBuffersMapped[currentImage], &ro, sizeof(ro));
}

VkVertexInputBindingDescription RaycasterVertex::getBindingDescription() {
	return {.binding = 0,
			.stride = sizeof(RaycasterVertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
}

std::array<VkVertexInputAttributeDescription, 2>
RaycasterVertex::getAttributeDescriptions() {
	std::array<VkVertexInputAttributeDescription, 2> res;
	res[0] = {.location = 0,
			  .binding = 0,
			  .format = VK_FORMAT_R32G32_SFLOAT,
			  .offset = offsetof(RaycasterVertex, pos)};
	res[1] = {.location = 1,
			  .binding = 0,
			  .format = VK_FORMAT_R32G32_SFLOAT,
			  .offset = offsetof(RaycasterVertex, display)};
	return res;
}

void Level::cleanup() { vmaDestroyBuffer(allocator, buf, alloc); }

Level Level::create(VulkanContext &context, VoxelModelMetadata &metadata,
					u32 *voxels, u32 palette[256]) {
	Level level;
	level.allocator = context.allocator;
	level.extent = glm::vec3(metadata.width, metadata.height, metadata.depth);

	u64 dataSize =
		metadata.width * metadata.height * metadata.depth * sizeof(u8);
	CreateBufferInfo bufInfo{.size = dataSize,
							 .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
									  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							 .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							 .vmaFlags = 0};
	context.createBuffer(bufInfo, level.buf, level.alloc);

	VkBuffer staging;
	VmaAllocation stagingAlloc;
	bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	bufInfo.vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	context.createBuffer(bufInfo, staging, stagingAlloc);

	u8 *data;
	vmaMapMemory(context.allocator, stagingAlloc, (void **)&data);
	for (size_t i = 0; i < dataSize; i++) {
		data[i] = 0;
	}
	for (size_t i = 0; i < metadata.amount_voxels; i++) {
		u8 w = voxels[i] & 0xff;
		u8 h = (voxels[i] >> 8) & 0xff;
		u8 d = (voxels[i] >> 16) & 0xff;
		u8 index = (voxels[i] >> 24) & 0xff;
		data[(metadata.width * metadata.height) * d + metadata.width * h + w] =
			index;
	}
	for (size_t i = 0; i < 256; i++) {
		level.palette[i] = palette[i];
	}
	vmaUnmapMemory(context.allocator, stagingAlloc);
	context.copyBuffer(staging, level.buf, dataSize);
	vmaDestroyBuffer(context.allocator, staging, stagingAlloc);

	return level;
}
