#pragma once
#include <functional>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef double f64;
typedef float f32;

struct Hash {
	u32 prime;
	u32 startChars;
	u32 endChars;
	size_t assetCount;
};

#define HASH_SIZE sizeof(Hash)

enum AssetType {
	IMAGE,
	MODEL,
	VOXEL_MODEL,
};

struct Entry {
	u64 hash;
	uintptr_t offset;
	uintptr_t size;
	AssetType type;

	bool operator<(const Entry &comp) const { return (hash < comp.hash); }

	bool operator==(const Entry &other) const { return (hash == other.hash); }
};

#define ENTRY_SIZE sizeof(Entry)

struct TextureMetadata {
	int width;
	int height;
	int bitDepth;
};

enum VertexAttrib {
	POS = 1,
	NORM = 2,
	TAN = 4,
	TEX = 8,
};

struct ModelMetadata {
	u64 vertexCount;
	u64 indexCount;
	u8 vertexAttributes;
};

struct VoxelModelMetadata {
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t amount_voxels;
};


struct Voxel {
	uint8_t pos[3];
	uint32_t color;
};
