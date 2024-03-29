#include "vox.h"
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>

std::vector<char> loadFile(std::string path);
std::string getName(uintptr_t &p, std::vector<char> &contents);
uint32_t getSize(uintptr_t &p, std::vector<char> &contents);

struct ChunkMetadata {
	std::string id;
	uint32_t size;
	uint32_t children_size;

	ChunkMetadata(uintptr_t &p, std::vector<char> &content) {
		id = getName(p, content);
		size = getSize(p, content);
	}
};
void ignoreChunk(ChunkMetadata chunk, uintptr_t &p,
				 std::vector<char> &contents);

uint8_t *vox_load(std::string path, uint32_t *width, uint32_t *height,
				  uint32_t *depth) {
	auto contents = loadFile(path);
	if (!contents.size()) {
		return nullptr;
	}

	uintptr_t p = 0;

	// Read header
	if (getName(p, contents) != "VOX ") {
		std::cerr << "File is not in VOX format!" << std::endl;
		return nullptr;
	}
	uint32_t version = getSize(p, contents);

	auto main = ChunkMetadata(p, contents);
	if (main.id != "MAIN" || main.size != 0) {
		std::cerr << "Vox file MAIN chunk malformed!" << std::endl;
		return nullptr;
	}

	auto next = ChunkMetadata(p, contents);
	if (next.id == "PACK") {
		ignoreChunk(next, p, contents);
		next = ChunkMetadata(p, contents);
	}

	if (next.id != "SIZE" || next.size != 12) {
		std::cerr << "No SIZE chunk in VOX file!" << std::endl;
		return nullptr;
	}
	*width = getSize(p, contents);
	*height = getSize(p, contents);
	*depth = getSize(p, contents);

	uint32_t *data =
		(uint32_t *)calloc(*width * *height * *depth, sizeof(uint32_t));
	return (uint8_t *)data;
}

std::vector<char> loadFile(std::string path) {
	std::ifstream ifs(path, std::ios::binary | std::ios::ate);
	std::ifstream::pos_type pos = ifs.tellg();
	if (pos == 0) {
		return {};
	}
	ifs.seekg(0);
	std::vector<char> res(pos);
	ifs.read(res.data(), pos);
	ifs.close();

	return res;
}

std::string getName(uintptr_t &p, std::vector<char> &contents) {
	std::stringstream ss;
	ss << contents[p++] << contents[p++] << contents[p++] << contents[p++];
	return ss.str();
}

uint32_t getSize(uintptr_t &p, std::vector<char> &contents) {
	uint32_t res = 0;
	for (size_t i = 0; i < 4; i++) {
		res = (res << 8) + contents[p++];
	}
	return res;
}

void ignoreChunk(ChunkMetadata metadata, uintptr_t &p,
				 std::vector<char> &contents) {
	p += metadata.size;
}
