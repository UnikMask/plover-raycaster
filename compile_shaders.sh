#!/bin/sh

cd $(dirname "${BASH_SOURCE[0]}")

[ -d "$HOME/vulkan" ] && source ~/vulkan/1.3.239.0/setup-env.sh
[ -d "$HOME/VulkanSDK" ] && source ~/VulkanSDK/1.3.239.0/setup-env.sh

[ ! -d "resources/spirv" ] && mkdir resources/spirv

SHADERS_DIR="resources/shaders"
SPIRV_DIR="resources/spirv"
for file in $(ls $SHADERS_DIR); do
    glslc $SHADERS_DIR/$file -o $SPIRV_DIR/$file.spv
done
