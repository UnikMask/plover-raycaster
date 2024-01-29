#pragma once

#include <plover/plover.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Texture.h"

const int MAX_TEXTURES = 3;

struct RaycasterContext {
  public:
	// Raycaster information
	Texture *lvlTex;
	std::vector<Texture *> textures;

	VulkanContext *context;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VmaAllocation> uniformBufferAllocations;
	std::vector<void *> uniformBuffersMapped;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	RaycasterContext(VulkanContext &context);
	~RaycasterContext();

	void updateUniform();
	void createUniform();
	void createDescriptorSets();
	void createRaycasterPipeline();
	void createDescriptorSetLayout();
};

struct RaycasterUniform {
	glm::vec3 cameraPos;
	glm::vec3 cameraUp;
	glm::vec3 cameraLeft;
	float fov;
	float aspectRatio;
	float minDistance;
	float maxDistance;
};
