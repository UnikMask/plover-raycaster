#include "vox.h"
#include "writer.h"
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

uint8_t *vox_load(std::string path, uint32_t *width, uint32_t *height,
				  uint32_t *depth, uint32_t *amount_voxels) {
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
		std::cerr << "No SIZE chunk in VOX file or size malformed!"
				  << std::endl;
		return nullptr;
	}
	*width = getSize(p, contents);
	*height = getSize(p, contents);
	*depth = getSize(p, contents);

	// Get voxel data
	next = ChunkMetadata(p, contents);
	if (next.id != "XYZI") {
		std::cerr << "No XYZI chunk in VOX file - file malformed!" << std::endl;
		return nullptr;
	}
	*amount_voxels = getSize(p, contents);
	uint32_t *voxels = (uint32_t *)malloc(*amount_voxels * sizeof(uint32_t));
	for (size_t i = 0; i < *amount_voxels; i++) {
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
	uint32_t colors[256];
	for (size_t i = 0; i < 256; i++) {
		colors[i] = getSize(p, contents);
	}
	contents.clear();

	Voxel *data = (Voxel *)calloc(*amount_voxels, sizeof(Voxel));
	for (size_t i = 0; i < *amount_voxels; i++) {
		data[i].pos[0] = voxels[i] & 0xff;
		data[i].pos[1] = (voxels[i] >> 8) & 0xff;
		data[i].pos[2] = (voxels[i] >> 16) & 0xff;
		data[i].color = colors[voxels[i] >> 24];
	}
	free(voxels);
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
		res += contents[p++] << (i * 8);
	}
	return res;
}

void ignoreChunk(ChunkMetadata metadata, uintptr_t &p,
				 std::vector<char> &contents) {
	p += metadata.size;
}
