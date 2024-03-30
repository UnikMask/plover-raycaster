#pragma once

#include "glm/fwd.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "file_utils.h"
#include "lapwing.h"

struct Writer {
	std::ofstream *assets;
	size_t ToCLength;
	std::unordered_map<std::string, AssetType> extensionsToType;

	Writer(std::string filename, size_t assetCount);

	void writeHash(Hash hash);

	void writeTableOfContents(Entry *ToC);

	uintptr_t writeAsset(Entry &content, std::string path);

	~Writer();

  private:
	uintptr_t writeImage(Entry &content, std::string path);
	uintptr_t writeModel(Entry &content, std::string path);
	uintptr_t writeVoxelModel(Entry &content, std::string path);
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 texCoord;

	bool operator==(const Vertex &other) const {
		return pos == other.pos && normal == other.normal &&
			   texCoord == other.texCoord;
	}
};

namespace std {
template <> struct hash<Vertex> {
	size_t operator()(Vertex const &vertex) const {
		return ((hash<glm::vec3>()(vertex.pos) ^
				 (hash<glm::vec3>()(vertex.normal) << 1)) >>
				1) ^
			   (hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};
} // namespace std
