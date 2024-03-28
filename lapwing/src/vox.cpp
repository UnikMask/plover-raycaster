#include "vox.h"
#include <cstdint>
#include <fstream>
#include <ios>
#include <vector>

uint8_t *vox_load(std::string path, uint32_t *width, uint32_t *height,
				  uint32_t *depth) {
	// Copy vox file content into array
	std::ifstream ifs(path, std::ios::binary | std::ios::ate);
	std::ifstream::pos_type pos = ifs.tellg();
	if (pos == 0) {
		return nullptr;
	}
	ifs.seekg(0);
	std::vector<char> res(pos);
	ifs.read(res.data(), pos);
	ifs.close();

	return nullptr;
}
