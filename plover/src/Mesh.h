#pragma once
#include <plover/plover.h>

#include "Texture.h"

#include <vma/vk_mem_alloc.h>
#include "glfw.h"
#include <array>

struct VulkanContext;

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 texCoord;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
	}

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tangent);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};

struct MeshUniform {
	glm::mat4 model;
};

struct Mesh {
	Vertex* vertices;
	u64 vertexCount;
	uint32_t* indices;
	u64 indexCount;

	VkBuffer vertexBuffer;
	VmaAllocation vertexAllocation;

	VkBuffer indexBuffer;
	VmaAllocation indexAllocation;

	glm::mat4 transform;

	std::vector<VkDescriptorSet> uniformDescriptorSets;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VmaAllocation> uniformBufferAllocations;
	std::vector<void*> uniformBuffersMapped;

	size_t materialId;

	void createUniform(VulkanContext& context);
	void updateUniformBuffer(uint32_t currentImage);

	void cleanup(VulkanContext& context);
};

void createVertexBuffer(VulkanContext& context,
						const Vertex* vertices,
						u64 vertexCount,
						VkBuffer& buffer,
						VmaAllocation& allocation);

void createIndexBuffer(VulkanContext& context,
					   const uint32_t* indices,
					   u64 indexCount,
					   VkBuffer& buffer,
					   VmaAllocation& allocation);

size_t createMesh(VulkanContext& context,
				  Vertex* vertices,
				  u64 vertexCount,
				  uint32_t* indices,
				  u64 indexCount,
				  size_t materialId);

void createMeshDescriptorSetLayout(VulkanContext& context);
