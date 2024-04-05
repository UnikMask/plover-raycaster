#pragma once

#include "lapwing.h"
#include <cstdint>
#include <string>

uint32_t *vox_load(std::string path, VoxelModelMetadata *metadata, u32 *palette);
