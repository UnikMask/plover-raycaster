#pragma once

#include <plover/plover.h>
#include <lapwing.h>

#include <unordered_map>
#include <string>
#include <fstream>

struct ModelData {
	u8* vertices;
	u8* indices;
};

struct AssetLoader {
	std::unordered_map<u64, Entry> tableOfContents;
	std::ifstream* assets;
	Hash hash;

	AssetLoader();

	char* loadTexture(const char* name, TextureMetadata* info);
	ModelData loadModel(const char* name, ModelMetadata* info);
	
	u64 hashAsset(const char* name);

	void cleanup();
};
