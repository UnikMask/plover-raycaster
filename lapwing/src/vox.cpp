#include "vox.h"
#include "lapwing.h"
#include <cstddef>
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
		children_size = getSize(p, content);
	}
};
void ignoreChunk(ChunkMetadata chunk, uintptr_t &p,
				 std::vector<char> &contents);

u32 *vox_load(std::string path, VoxelModelMetadata *metadata, u32 *palette) {
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
	u32 version = getSize(p, contents);

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
		std::cerr << "No SIZE chunk in VOX file or size malformed!"
				  << std::endl;
		return nullptr;
	}
	metadata->width = getSize(p, contents);
	metadata->height = getSize(p, contents);
	metadata->depth = getSize(p, contents);

	// Get voxel data
	next = ChunkMetadata(p, contents);
	if (next.id != "XYZI") {
		std::cerr << "No XYZI chunk in VOX file - file malformed!" << std::endl;
		return nullptr;
	}
	metadata->amount_voxels = getSize(p, contents);
	u32 *voxels = (u32 *)malloc(metadata->amount_voxels * sizeof(u32));
	for (size_t i = 0; i < metadata->amount_voxels; i++) {
		voxels[i] = getSize(p, contents);
	}

	// Get palette
	next = ChunkMetadata(p, contents);
	for (; p < contents.size() && next.id != "RGBA";
		 next = ChunkMetadata(p, contents)) {
		ignoreChunk(next, p, contents);
	}
	if (p == contents.size()) {
		std::cerr << "No RGBA chunk in VOX file - file malformed!" << std::endl;
		free(voxels);
		return nullptr;
	}
	if (next.size != 256 * 4) {
		std::cerr << "Malformed VOX file palette!" << std::endl;
		free(voxels);
		return nullptr;
	}
	for (size_t i = 0; i <= 255; i++) {
		palette[i + 1] = getSize(p, contents);
	}
	palette[0] = getSize(p, contents);
	contents.clear();
	std::cout << "Vox file loading finished!" << std::endl;
	return voxels;
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
		res += contents[p++] << (i * 8);
	}
	return res;
}

void ignoreChunk(ChunkMetadata metadata, uintptr_t &p,
				 std::vector<char> &contents) {
	p += metadata.size;
}
