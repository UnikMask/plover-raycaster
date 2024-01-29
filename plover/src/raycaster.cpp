#include "raycaster.h"
#include "Texture.h"
#include "VulkanContext.h"
#include <array>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

RaycasterContext::RaycasterContext(VulkanContext &context) {
	this->context = &context;
	createDescriptorSetLayout();
	createUniform();
	createDescriptorSets();
	createRaycasterPipeline();
}

RaycasterContext::~RaycasterContext() {
	vkDestroyDescriptorSetLayout(context->device, descriptorSetLayout, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vmaDestroyBuffer(context->allocator, uniformBuffers[i],
						 uniformBufferAllocations[i]);
	}
	vkDestroyPipeline(context->device, pipeline, nullptr);
	vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
	context = nullptr;
}

void RaycasterContext::createUniform() {
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
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = 0};

	VkDescriptorSetLayoutBinding textureMappings{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = MAX_TEXTURES,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = 0};

	std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
		uboBinding, levelBinding, textureMappings};

	VkDescriptorSetLayoutCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = bindings.size(),
		.pBindings = bindings.data()};
	if (vkCreateDescriptorSetLayout(context->device, &info, nullptr,
									&descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error(
			"Failed to create raycaster descriptor set layout!");
	}
}

void RaycasterContext::createDescriptorSets() {
	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	context->descriptorAllocator.allocate(
		MAX_FRAMES_IN_FLIGHT, descriptorSets.data(), descriptorSetLayout);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{.buffer = uniformBuffers[i],
										  .offset = 0,
										  .range = sizeof(RaycasterUniform)};
		// Generate level texture image info
		VkDescriptorImageInfo lvlInfo{
			.sampler = nullptr,
			.imageView = lvlTex->imageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		// Generate block texture image info
		VkDescriptorImageInfo texInfo[MAX_TEXTURES];
		for (int j = 0; j < MAX_TEXTURES; j++) {
			texInfo[j] = VkDescriptorImageInfo{
				.sampler = nullptr,
				.imageView = textures[j % textures.size()]->imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		}

		// Camera uniform buffer information
		VkWriteDescriptorSet uboInfo{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &bufferInfo};

		// Level texture information
		VkWriteDescriptorSet levelInfo{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = descriptorSets[i],
			.dstBinding = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &lvlInfo};

		// Block texture information
		VkWriteDescriptorSet texDsInfo{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = descriptorSets[i],
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = MAX_TEXTURES,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = texInfo};

		VkWriteDescriptorSet infoArray[3] = {uboInfo, levelInfo, texDsInfo};
		vkUpdateDescriptorSets(context->device, 3, infoArray, 0, nullptr);
	}
}

void RaycasterContext::createRaycasterPipeline() {
	VkDescriptorSetLayout layouts[1] = {descriptorSetLayout};

	PipelineCreateInfo createInfo{
		.useDepthBuffer = true,
		.doCulling = true,
		.wireframeMode = false,
		.subpass = 2,
		.vertexShaderPath = "../resources/spirv/raycaster.vert.spv",
		.fragmentShaderPath = "../resources/spirv/raycaster.frag.spv",
		.descriptorSetLayoutCount = 1,
		.bindingDescriptionCount = 0,
		.pBindingDescriptions = nullptr,
		.attributeDescriptionCount = 0,
		.pAttributeDescriptions = nullptr};

	context->createGraphicsPipeline(createInfo, pipeline, pipelineLayout);
}
