#include "AssetLoader.h"
#include "Mesh.h"
#include "lapwing.h"

#include <cstdlib>
#include <stdexcept>
#include <string.h>
#include <math.h>
#include <iostream>

AssetLoader::AssetLoader() {
	assets = new std::ifstream("../resources/assets.plv", std::ifstream::in | std::ifstream::binary);
	if (assets->fail()) {
		throw std::runtime_error("The assets file does not exist.");
	}
	assets->read((char*)&hash, HASH_SIZE);

	for (size_t i = 0; i < hash.assetCount; i++)
	{
		Entry entry{};
		assets->read((char*)&entry, ENTRY_SIZE);
		
		tableOfContents.insert({entry.hash, entry});
	}
}

char* AssetLoader::loadTexture(const char* name, TextureMetadata* info) {
	u64 assetHash = hashAsset(name);
	auto entryPair = tableOfContents.find(assetHash);

	if (entryPair != tableOfContents.end()) {
		Entry entry = entryPair->second;
		assets->seekg(entry.offset, std::ios_base::beg);
		assets->read((char*)info, sizeof(TextureMetadata));

		// TODO (yigit): Change this to use an arena once they are implemented.
		char* texture = new char[entry.size - sizeof(TextureMetadata)];

		assets->read(texture, entry.size - sizeof(TextureMetadata));
		return texture;
	} else {
		throw std::runtime_error("Asset not found.\n");
	}
}

ModelData AssetLoader::loadModel(const char* name, ModelMetadata* info) {
	u64 assetHash = hashAsset(name);
	auto entryPair = tableOfContents.find(assetHash);
	ModelData data;

	if (entryPair != tableOfContents.end()) {
		Entry entry = entryPair->second;
		assets->seekg(entry.offset, std::ios_base::beg);
		assets->read((char*)info, sizeof(ModelMetadata));

		char* vertices = new char[info->vertexCount * sizeof(Vertex)];
		assets->read(vertices, info->vertexCount * sizeof(Vertex));

		char* indices = new char[info->indexCount * sizeof(uint32_t)];
		assets->read(indices, info->indexCount * sizeof(uint32_t));

		data.vertices = (u8*) vertices;
		data.indices = (u8*) indices;
	} else {
		throw std::runtime_error("Asset not found.\n");
	}

	return data; 
}

u32 *AssetLoader::loadVoxelModel(const char *name, VoxelModelMetadata *info, u32 *palette) {
    u64 assetHash = hashAsset(name);
    auto entryPair = tableOfContents.find(assetHash);
    
    if (entryPair == tableOfContents.end()) {
        throw std::runtime_error("Asset not found.\n");
        return nullptr;
    }

    Entry entry = entryPair->second;
    assets->seekg(entry.offset, std::ios_base::beg);
    assets->read((char *) info, sizeof(VoxelModelMetadata));

    u32 *voxels = (u32*) malloc(info->amount_voxels * sizeof(u32));
    assets->read((char *) voxels, info->amount_voxels * sizeof(u32));
    assets->read((char *) palette, sizeof(u32[256]));
    return voxels;
}

u64 AssetLoader::hashAsset(const char* name) {
	u32 power = 1;
	u64 value = strlen(name);
	u64 assetLen = strlen(name);

	for (size_t i = 0; i < hash.startChars; i++) {
		value += (static_cast<u64>(name[i % assetLen]) - 33) * static_cast<u64>(std::pow(hash.prime, power++));
	}

	for (size_t i = 0; i < hash.endChars; i++) {
		value += (static_cast<u64>(name[(assetLen - i) % assetLen]) - 33) * static_cast<u64>(std::pow(hash.prime, power++));
	}

	return value;
}

void AssetLoader::cleanup() {
	assets->close();
}
