#pragma once

#include <plover/plover.h>

#include "AssetLoader.h"

#include <vma/vk_mem_alloc.h>

struct VulkanContext;

enum BitmapFormat {
	G8,		// 8-bit gray
	RGBA8,	// 8-bit rgba
	SRGBA8, // 8-bit srgba
};

inline u32 stride(BitmapFormat format);
inline VkFormat vulkanFormat(BitmapFormat format);
struct Bitmap {
	void *pixels;
	u32 width;
	u32 height;
	BitmapFormat format;

	inline u32 stride() { return ::stride(format); }

	inline VkFormat vulkanFormat() {
        return ::vulkanFormat(format);
	}

	inline u32 index(u32 x, u32 y) { return (y * width + x) * stride(); }

	void writeGrayscale(u8 value, u32 x, u32 y);
	void writeRGBA(UVec4 color, u32 x, u32 y);
	void clear();
};

struct VoxelMap {
	void *voxels;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	BitmapFormat format;

	inline u32 stride() { return ::stride(format); }

	inline VkFormat vulkanFormat() {
        return ::vulkanFormat(format);
	}

	inline u32 index(u32 x, u32 y, u32 z) { 
        return z * (width * height) + y * width + x; 
    }

	VoxelMap(u32 width, u32 height, u32 depth, BitmapFormat format);
	~VoxelMap();
	void writeGrayscale(u8 value, u32 x, u32 y, u32 z);
	void writeRGBA(UVec4 color, u32 x, u32 y, u32 z);
};

void createBitmap(Bitmap *bitmap, u32 width, u32 height, BitmapFormat format);

struct Texture {
	VkFormat format;
	VkDeviceSize imageSize;

	VkImage image;
	VmaAllocation allocation;
	VkImageView imageView;
	VkSampler sampler;

	void copyBitmap(VulkanContext &context, Bitmap bitmap);
	void copyVoxelmap(VulkanContext &context, VoxelMap voxelmap);
	void cleanup(VulkanContext &context);
};

void createTexture(VulkanContext &context, Bitmap bitmap, Texture &texture);
void createTexture(VulkanContext &context, VoxelMap voxelmap, Texture &texture);
void createImageTexture(VulkanContext &context, AssetLoader loader,
						Texture &texture, const char *name,
						BitmapFormat format);

struct ArrayTexture {
	VkFormat format;
	VkDeviceSize imageSize;
	u32 layers;

	VkImage image;
	VmaAllocation allocation;
	VkImageView imageView;
	VkSampler sampler;

	void copyBitmaps(VulkanContext &context, Bitmap *bitmaps);
	void cleanup(VulkanContext &context);
};

struct ArrayTextureCreateInfo {
	Bitmap *bitmaps;
	u32 layers;
};
void createArrayTexture(VulkanContext *context, ArrayTextureCreateInfo info,
						ArrayTexture *texture);

inline u32 stride(BitmapFormat format) {
	switch (format) {
	case G8:
		return 1;
	case RGBA8:
		return 4;
	case SRGBA8:
		return 4;
	}
}
inline VkFormat vulkanFormat(BitmapFormat format) {
		switch (format) {
		case G8:
			return VK_FORMAT_R8_UNORM;
		case RGBA8:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case SRGBA8:
			return VK_FORMAT_R8G8B8A8_SRGB;
		}
}
