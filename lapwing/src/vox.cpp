#include "vox.h"
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
	size_t num_voxels = getSize(p, contents);
	uint32_t *voxels = (uint32_t *)malloc(num_voxels * sizeof(uint32_t));
	for (size_t i = 0; i < num_voxels; i++) {
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

	uint32_t *data =
		(uint32_t *)calloc(*width * *height * *depth, sizeof(uint32_t));
	for (size_t i = 0; i < num_voxels; i++) {
		uint8_t x = voxels[i] & 0xff;
		uint8_t y = (voxels[i] >> 8) & 0xff;
		uint8_t z = (voxels[i] >> 16) & 0xff;
		uint8_t index = voxels[i] >> 24;

		data[(*width * *height) * z + *width * y + x] = colors[index];
	}
	free(voxels);

	auto print_vox = [&](uint8_t w, uint8_t h, uint8_t d) {
		std::string reset = "\033[1;0m";
		uint32_t color = data[(*width * *height) * d + (*width) * h + w];
		if (!(color >> 24)) {
			std::cout << " ";
		} else {
			uint8_t r = color & 0xff;
			uint8_t g = (color >> 8) & 0xff;
			uint8_t b = (color >> 16) & 0xff;
			std::cout << "\x1b[38;2;" << (int)r << ";" << (int)g << ";"
					  << (int)b << "m"
					  << "X";
		}
	};

	for (uint8_t d = 0; d < *depth; d++) {
		for (uint8_t h = 0; h < *height; h++) {
			for (uint8_t w = 0; w < *width; w++) {
				print_vox(w, h, d);
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

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
