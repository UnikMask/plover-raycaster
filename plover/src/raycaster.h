#pragma once

#include <array>
#include <plover/plover.h>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Texture.h"
#include "glm/fwd.hpp"
#include "lapwing.h"

const int MAX_TEXTURES = 3;

struct Level {
	// Voxel allocs
	VkBuffer buf;
	VmaAllocation alloc;
	glm::ivec3 extent;

	// Palette
	uint32_t palette[256];

	static Level create(VulkanContext &context, VoxelModelMetadata &metadata,
						u32 *voxels, u32 palette[256]);
	VkDeviceSize size() { return extent.x * extent.y * extent.z; }
	void cleanup();

  private:
	VmaAllocator allocator;
};

struct RaycasterContext {
  public:
	// Raycaster information
	Level level;
	VkBuffer levelExtrasBuf;
	VmaAllocation levelExtrasAlloc;

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
	RaycasterContext(Level level, VulkanContext *context);
	~RaycasterContext();

  private:
	void createUniformBuffers();
	void createVertexBuffer();
	void createDescriptorSets();
	void createLevelExtrasBuf();
	void uploadToExtrasUniform();
	void createRaycasterPipeline();
	void createDescriptorSetLayout();
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

struct RaycasterExtrasUniform {
	alignas(16) glm::ivec3 extent;
	alignas(4) uint32_t palette[256];
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
