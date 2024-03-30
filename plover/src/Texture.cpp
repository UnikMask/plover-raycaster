#include "Texture.h"
#include "VulkanContext.h"
#include "lapwing.h"
#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan_core.h>

void createBitmap(Bitmap *bitmap, u32 width, u32 height, BitmapFormat format) {
	bitmap->width = width;
	bitmap->height = height;
	bitmap->format = format;
	bitmap->pixels = new u8[width * height * bitmap->stride()] { 0 };
}

void Bitmap::writeGrayscale(u8 value, u32 x, u32 y) {
	assert(x >= 0 && y >= 0);
	assert(x < width * stride());
	assert(x < height * stride());
	
	switch (format) {
		case G8:
			((u8 *)pixels)[index(x, y)] = value;
			break;
		case RGBA8:
		case SRGBA8:
			if (value > 0) {
				((u8 *)pixels)[index(x, y)]     = 255;
				((u8 *)pixels)[index(x, y) + 1] = 255;
				((u8 *)pixels)[index(x, y) + 2] = 255;
				((u8 *)pixels)[index(x, y) + 3] = value;
			}
			if (((u8 *)pixels)[index(x, y) + 3] > 255) {
				((u8 *)pixels)[index(x, y) + 3] = 255;
			}
			break;
	}
}

void Bitmap::writeRGBA(UVec4 color, u32 x, u32 y) {
	assert(x >= 0 && y >= 0);
	assert(x < width * stride());
	assert(x < height * stride());
	
	switch (format) {
		case G8:
			((u8 *)pixels)[index(x, y)] = color.a;
			break;
		case RGBA8:
		case SRGBA8:
			((u8 *)pixels)[index(x, y)]     = color.r;
			((u8 *)pixels)[index(x, y) + 1] = color.g;
			((u8 *)pixels)[index(x, y) + 2] = color.b;
			((u8 *)pixels)[index(x, y) + 3] = color.a;
			break;
	}
}

void createTexture(VulkanContext& context, Bitmap bitmap, Texture& texture) {
	texture.imageSize = bitmap.width * bitmap.height * bitmap.stride();
	texture.format = bitmap.vulkanFormat();

	CreateImageInfo imageInfo{};
	imageInfo.width = bitmap.width;
	imageInfo.height = bitmap.height;
	imageInfo.format = texture.format;
	imageInfo.layers = 1;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageInfo.vmaFlags = static_cast<VmaAllocationCreateFlagBits>(0);
	context.createImage(imageInfo, texture.image, texture.allocation);

	texture.copyBitmap(context, bitmap);

	CreateImageViewInfo imageViewInfo{};
	imageViewInfo.image = texture.image;
	imageViewInfo.format = texture.format;
	imageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.layers = 1;
	context.createImageView(imageViewInfo, &texture.imageView);

	// Create sampler
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(context.physicalDevice, &supportedFeatures);
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(context.physicalDevice, &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	if (supportedFeatures.samplerAnisotropy == VK_TRUE) {
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	}
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(context.device, &samplerInfo, nullptr, &(texture.sampler)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void createImageTexture(VulkanContext& context,
						AssetLoader loader,
						Texture& texture,
						const char *name,
						BitmapFormat format) {
	TextureMetadata info{};

	Bitmap bitmap{};
	bitmap.pixels = loader.loadTexture(name, &info);
	bitmap.width = info.width;
	bitmap.height = info.height;
	bitmap.format = format;

	createTexture(context, bitmap, texture);
}

void Texture::copyBitmap(VulkanContext& context, Bitmap bitmap) {
	assert(bitmap.width * bitmap.height * bitmap.stride() == imageSize
		   && "Image size mismatch when updating texture!");
	assert(bitmap.vulkanFormat() == format
		   && "Format mismatch when updating texture!");

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	CreateBufferInfo stagingCreateInfo{};
	stagingCreateInfo.size = imageSize;
	stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingCreateInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	stagingCreateInfo.vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	context.createBuffer(stagingCreateInfo, stagingBuffer, stagingBufferAllocation);

	void* data;
	vmaMapMemory(context.allocator, stagingBufferAllocation, &data);
	memcpy(data, bitmap.pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(context.allocator, stagingBufferAllocation);

	// TODO(oliver): These all create and submit a new command buffer.  Should change this to use a single command buffer
	// TODO(oliver): Change these to non-array versions!
	context.transitionImageLayout(image,
								  format,
								  VK_IMAGE_LAYOUT_UNDEFINED,
								  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								  1);
	context.copyBufferToImage(stagingBuffer,
							  image,
							  bitmap.width,
							  bitmap.height,
							  imageSize,
							  1);
	context.transitionImageLayout(image,
								  format,
								  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								  1);

	vmaDestroyBuffer(context.allocator,
					 stagingBuffer,
					 stagingBufferAllocation);
}

void Texture::copyVoxelmap(VulkanContext &context, VoxelMap voxelmap) {
    assert(voxelmap.width * voxelmap.height * voxelmap.stride() == imageSize 
           && "Image size mismatch when updating texture!");
    assert(voxelmap.vulkanFormat() == format 
           && "Format mismatch when updating texture!");

    VkBuffer stagingBuf;
    VmaAllocation stagingBufAlloc;

    CreateBufferInfo stagingInfo {
        .size = imageSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT};

    context.createBuffer(stagingInfo, stagingBuf, stagingBufAlloc);

    void *data;
    vmaMapMemory(context.allocator, stagingBufAlloc, &data);
    memcpy(data, voxelmap.voxels, imageSize);
    vmaUnmapMemory(context.allocator, stagingBufAlloc);

    context.transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
    context.copyBufferToImage(stagingBuf, image, voxelmap.width, voxelmap.height, voxelmap.depth, imageSize, 1);
    context.transitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, 1);
}

void Texture::cleanup(VulkanContext& context) {
	vkDestroySampler(context.device, sampler, nullptr);
	vkDestroyImageView(context.device, imageView, nullptr);
	vmaDestroyImage(context.allocator, image, allocation);
}

void createArrayTexture(VulkanContext *context, ArrayTextureCreateInfo info, ArrayTexture *texture) {
	texture->imageSize = info.bitmaps[0].width * info.bitmaps[0].height * info.bitmaps[0].stride();
	texture->format = info.bitmaps[0].vulkanFormat();
	texture->layers = info.layers;

	CreateImageInfo imageInfo{};
	imageInfo.width = info.bitmaps[0].width;
	imageInfo.height = info.bitmaps[0].height;
	imageInfo.format = texture->format;
	imageInfo.layers = info.layers;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageInfo.vmaFlags = static_cast<VmaAllocationCreateFlagBits>(0);
	context->createImage(imageInfo, texture->image, texture->allocation);

	texture->copyBitmaps(*context, info.bitmaps);

	CreateImageViewInfo imageViewInfo{};
	imageViewInfo.image = texture->image;
	imageViewInfo.format = texture->format;
	imageViewInfo.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	imageViewInfo.layers = info.layers;
	context->createImageView(imageViewInfo, &texture->imageView);

	// Create sampler
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(context->physicalDevice, &supportedFeatures);
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(context->physicalDevice, &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	if (supportedFeatures.samplerAnisotropy == VK_TRUE) {
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	}
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(context->device, &samplerInfo, nullptr, &(texture->sampler)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture->sampler!");
	}
}

void ArrayTexture::copyBitmaps(VulkanContext& context, Bitmap *bitmaps) {
	assert(bitmaps != NULL && "Bitmap cannot be NULL!");
	assert(layers >= 1 && "Must have one or more layers to copy!");
	assert(bitmaps[0].width * bitmaps[0].height * bitmaps[0].stride() == imageSize
		   && "Image size mismatch when updating texture!");
	assert(bitmaps[0].vulkanFormat() == format
		   && "Format mismatch when updating texture!");

	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;

	CreateBufferInfo stagingCreateInfo{};
	stagingCreateInfo.size = imageSize * layers;
	stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingCreateInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	stagingCreateInfo.vmaFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	context.createBuffer(stagingCreateInfo, stagingBuffer, stagingBufferAllocation);

	void* data;
	vmaMapMemory(context.allocator, stagingBufferAllocation, &data);
	for (u32 layer = 0; layer < layers; layer++) {
		memcpy((u8*) data + layer * imageSize, bitmaps[layer].pixels, static_cast<size_t>(imageSize));
	}
	vmaUnmapMemory(context.allocator, stagingBufferAllocation);

	// TODO(oliver): These all create and submit a new command buffer.  Should change this to use a single command buffer
	context.transitionImageLayout(image,
								  format,
								  VK_IMAGE_LAYOUT_UNDEFINED,
								  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								  layers);
	context.copyBufferToImage(stagingBuffer,
							  image,
							  bitmaps[0].width,
							  bitmaps[0].height,
							  imageSize,
							  layers);
	context.transitionImageLayout(image,
								  format,
								  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								  layers);

	vmaDestroyBuffer(context.allocator,
					 stagingBuffer,
					 stagingBufferAllocation);
}

void ArrayTexture::cleanup(VulkanContext& context) {
	vkDestroySampler(context.device, sampler, nullptr);
	vkDestroyImageView(context.device, imageView, nullptr);
	vmaDestroyImage(context.allocator, image, allocation);
}

VoxelMap::VoxelMap(u32 width, u32 height, u32 depth, BitmapFormat format) {
    this->format = format;
    this->width = width, this->height = height, this->depth = depth;
    this->voxels = malloc(height * width * depth * stride());
}

VoxelMap::~VoxelMap() {
    free(this->voxels);
}

void VoxelMap::writeGrayscale(u8 value, u32 x, u32 y, u32 z) {
    switch (format) {
        case G8:
         ((u8*) voxels)[index(x, y, z)] = value;
            break;
        case RGBA8:
        case SRGBA8:
         ((u8*) voxels)[index(x, y, z) * stride()] = value;
         ((u8*) voxels)[index(x, y, z) * stride() + 1] = value;
         ((u8*) voxels)[index(x, y, z) * stride() + 2] = value;
         ((u8*) voxels)[index(x, y, z) * stride() + 3] = value;
    }
}


void VoxelMap::writeRGBA(UVec4 color, u32 x, u32 y, u32 z) {
    switch (format) {
        case G8:
         ((u8*) voxels)[index(x, y, z)] = color.a;
            break;
        case RGBA8:
        case SRGBA8:
         ((u8*) voxels)[index(x, y, z) * stride()] = color.r;
         ((u8*) voxels)[index(x, y, z) * stride() + 1] = color.g;
         ((u8*) voxels)[index(x, y, z) * stride() + 2] = color.b;
         ((u8*) voxels)[index(x, y, z) * stride() + 3] = color.a;
    }
}


void createTexture(VulkanContext &context, VoxelMap voxelmap, Texture &texture) {
    texture.imageSize = voxelmap.width * voxelmap.height * voxelmap.depth * voxelmap.stride();
    texture.format = voxelmap.vulkanFormat();

    CreateImageInfo imageInfo {
        .width = voxelmap.width,
        .height = voxelmap.height,
        .depth = voxelmap.depth,
        .layers = 1,
        .format = texture.format,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .vmaFlags = (VmaAllocationCreateFlagBits) 0};

    context.createImage(imageInfo, texture.image, texture.allocation);
    texture.copyVoxelmap(context, voxelmap);

    CreateImageViewInfo imageViewInfo {
        .image = texture.image,
        .format = texture.format,
        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .viewType = VK_IMAGE_VIEW_TYPE_3D,
        .layers = 1
    };
    context.createImageView(imageViewInfo, &texture.imageView);

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(context.physicalDevice, &supportedFeatures);
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(context.physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .compareOp = VK_COMPARE_OP_ALWAYS,
    };

    if (supportedFeatures.samplerAnisotropy == VK_TRUE) {
        samplerInfo.anisotropyEnable = VK_TRUE,
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    }

    if (vkCreateSampler(context.device, &samplerInfo, nullptr, &texture.sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
    }
}
