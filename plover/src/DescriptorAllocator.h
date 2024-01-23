#pragma once

#include <plover/plover.h>

#include "glfw.h"

const u32 DEFAULT_DESCRIPTOR_POOL_SIZE = 1000;

struct DescriptorAllocator {
	VkDevice device;

	VkDescriptorPool currentPool{ VK_NULL_HANDLE };
	std::vector<VkDescriptorPool> freePools;
	std::vector<VkDescriptorPool> usedPools;

	void init(VkDevice device);
	void cleanup();

	void allocate(uint32_t setCount, VkDescriptorSet* sets, VkDescriptorSetLayout layout);

	VkDescriptorPool getPool();
};
