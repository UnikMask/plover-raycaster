#pragma once

#include <plover/plover.h>

#include "AssetLoader.h"

#include <vma/vk_mem_alloc.h>

struct VulkanContext;

enum BitmapFormat {
	G8, // 8-bit gray
	RGBA8, // 8-bit rgba
	SRGBA8, // 8-bit srgba
};

struct Bitmap {
	void *pixels;
	u32 width;
	u32 height;
	BitmapFormat format;

	inline u32 stride() {
		switch (format) {
			case G8:
				return 1;
			case RGBA8:
				return 4;
			case SRGBA8:
				return 4;
		}
	}

	inline VkFormat vulkanFormat() {
		switch (format) {
			case G8:
				return VK_FORMAT_R8_UNORM;
			case RGBA8:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case SRGBA8:
				return VK_FORMAT_R8G8B8A8_SRGB;
		}
	}

	inline u32 index(u32 x, u32 y) {
		return (y * width + x) * stride();
	}

	void writeGrayscale(u8 value, u32 x, u32 y);
	void writeRGBA(UVec4 color, u32 x, u32 y);
	void clear();
};

void createBitmap(Bitmap *bitmap, u32 width, u32 height, BitmapFormat format);

struct Texture {
	VkFormat format;
	VkDeviceSize imageSize;

	VkImage image;
	VmaAllocation allocation;
	VkImageView imageView;
	VkSampler sampler;

	void copyBitmap(VulkanContext& context, Bitmap bitmap);
	void cleanup(VulkanContext& context);
};

void createTexture(VulkanContext& context, Bitmap bitmap, Texture& texture);
void createImageTexture(VulkanContext& context,
						AssetLoader loader,
						Texture &texture,
						const char *name,
						BitmapFormat format);

struct ArrayTexture {
	VkFormat format;
	VkDeviceSize imageSize;
	u32 layers;

	VkImage image;
	VmaAllocation allocation;
	VkImageView imageView;
	VkSampler sampler;

	void copyBitmaps(VulkanContext& context, Bitmap *bitmaps);
	void cleanup(VulkanContext& context);
};

struct ArrayTextureCreateInfo {
	Bitmap *bitmaps;
	u32 layers;
};
void createArrayTexture(VulkanContext *context,
						ArrayTextureCreateInfo info,
						ArrayTexture *texture);
