#pragma once

#include <array>
#include <plover/plover.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Texture.h"
#include "glm/fwd.hpp"

const int MAX_TEXTURES = 3;

struct RaycasterContext {
  public:
	// Raycaster information
	Texture lvlTex;
	// std::vector<Texture *> textures;

	VulkanContext *context;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VmaAllocation> uniformBufferAllocations;
	std::vector<void *> uniformBuffersMapped;
	VkBuffer vertexBuffer;
	VmaAllocation vertexBufferAlloc;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	void updateUniform(uint32_t currentImage);
	RaycasterContext(VulkanContext *context, uint32_t width, uint32_t height,
					 uint64_t seed);
	~RaycasterContext();

  private:
	void createUniformBuffers();
	void createVertexBuffer();
	void createDescriptorSets();
	void createRaycasterPipeline();
	void createDescriptorSetLayout();

	void createMap(uint32_t width, uint32_t height, uint64_t seed);
};

struct RaycasterUniform {
	alignas(16) glm::vec3 cameraPos;
	alignas(16) glm::vec3 cameraDir;
	alignas(16) glm::vec3 cameraUp;
	alignas(16) glm::vec3 cameraLeft;
	alignas(4) float fov;
	alignas(4) float aspectRatio;
	alignas(4) float minDistance;
	alignas(4) float maxDistance;
};

struct RaycasterVertex {
	glm::vec2 pos;
	glm::vec2 display;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2>
	getAttributeDescriptions();
};

const std::vector<RaycasterVertex> raycasterVertices{
	{{1, 1}, {1, 1}},	{{1, -1}, {1, -1}},	  {{-1, 1}, {-1, 1}},
	{{1, -1}, {1, -1}}, {{-1, -1}, {-1, -1}}, {{-1, 1}, {-1, 1}}};
