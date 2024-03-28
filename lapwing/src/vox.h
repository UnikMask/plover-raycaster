#pragma  once

#include <cstdint>
#include <string>

uint8_t *vox_load(std::string path, uint32_t* width, uint32_t* height, uint32_t* depth);
