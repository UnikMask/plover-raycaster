#include "writer.h"
#include "file_utils.h"
#include "lapwing.h"
#include "vox.h"
#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <obj/tiny_obj_loader.h>

Writer::Writer(std::string filename, size_t assetCount) {
	assets =
		new std::ofstream(filename, std::ofstream::out | std::ofstream::binary);
	ToCLength = assetCount;

	// Images
	extensionsToType[".png"] = AssetType::IMAGE;
	extensionsToType[".jpg"] = AssetType::IMAGE;
	extensionsToType[".jpeg"] = AssetType::IMAGE;
	extensionsToType[".bmp"] = AssetType::IMAGE;
	extensionsToType[".hdr"] = AssetType::IMAGE;
	extensionsToType[".psd"] = AssetType::IMAGE;
	extensionsToType[".tga"] = AssetType::IMAGE;
	extensionsToType[".gif"] = AssetType::IMAGE;
	extensionsToType[".pic"] = AssetType::IMAGE;
	extensionsToType[".pgm"] = AssetType::IMAGE;
	extensionsToType[".ppm"] = AssetType::IMAGE;

	// Models
	extensionsToType[".obj"] = AssetType::MODEL;

	// Voxel Models
	extensionsToType[".vox"] = AssetType::VOXEL_MODEL;
}

void Writer::writeHash(Hash hash) {
	assets->seekp(0, std::ios_base::beg);
	assets->write((char *)&hash, sizeof(Hash));
}

void Writer::writeTableOfContents(Entry *ToC) {
	assets->seekp(sizeof(Hash), std::ios_base::beg);
	assets->write((char *)ToC, ENTRY_SIZE * ToCLength);
}

uintptr_t Writer::writeImage(Entry &content, std::string path) {
	content.type = AssetType::IMAGE;
	TextureMetadata textureMetadata = {};
	if (fileExists(path)) {
		u8 *image =
			stbi_load(path.c_str(), &textureMetadata.width,
					  &textureMetadata.height, &textureMetadata.bitDepth, 0);
		assets->write((char *)&textureMetadata, sizeof(TextureMetadata));
		assets->write((char *)image, textureMetadata.width *
										 textureMetadata.height *
										 textureMetadata.bitDepth);
		stbi_image_free(image);
		content.size = sizeof(TextureMetadata) + textureMetadata.width *
													 textureMetadata.height *
													 textureMetadata.bitDepth;
		return content.offset + content.size;
	} else {
		std::cerr << "Asset file " << path << " does not exist." << std::endl;
	}
	return -1;
}

uintptr_t Writer::writeVoxelModel(Entry &content, std::string path) {
	content.type = AssetType::VOXEL_MODEL;
	VoxelModelMetadata modelMetadata{};
	if (!fileExists(path)) {
		std::cerr << "Asset file " << path << " does not exist." << std::endl;
		return -1;
	}
	u8 *model = vox_load(path, &modelMetadata.width, &modelMetadata.height,
						 &modelMetadata.depth, &modelMetadata.amount_voxels);
	if (model == nullptr) {
		return -1;
	}

	content.size = sizeof(VoxelModelMetadata) * modelMetadata.width *
				   modelMetadata.height * modelMetadata.depth;
	assets->write((char *)&modelMetadata, sizeof(VoxelModelMetadata));
	assets->write((char *)model, content.size);
	free(model);

	return content.offset + content.size;
}

uintptr_t Writer::writeModel(Entry &content, std::string path) {
	content.type = AssetType::MODEL;
	ModelMetadata modelMetadata = {};
	if (fileExists(path)) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
							  path.c_str())) {
			throw std::runtime_error(warn + err);
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto &shape : shapes) {
			int i = 0;
			for (const auto &index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
							  attrib.vertices[3 * index.vertex_index + 1],
							  attrib.vertices[3 * index.vertex_index + 2]};

				vertex.normal = {attrib.normals[3 * index.normal_index + 0],
								 attrib.normals[3 * index.normal_index + 1],
								 attrib.normals[3 * index.normal_index + 2]};

				vertex.tangent = {};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index +
											1] // NOTE(oliver): Flip to conform
											   // to OBJ
				};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] =
						static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);

				i++;
				if ((i % 3) == 0) { // NOTE(oliver): Performed per tri
					u64 indicesSize = indices.size();
					u32 i0 = indices[indicesSize - 3];
					u32 i1 = indices[indicesSize - 2];
					u32 i2 = indices[indicesSize - 1];
					Vertex &v0 = vertices[i0];
					Vertex &v1 = vertices[i1];
					Vertex &v2 = vertices[i2];
					glm::vec3 v0v1 = v1.pos - v0.pos;
					glm::vec3 v0v2 = v2.pos - v0.pos;

					glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
					glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;
					float k =
						1 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

					glm::vec3 tangent = glm::vec3(
						k * (deltaUV2.y * v0v1.x - deltaUV1.y * v0v2.x),
						k * (deltaUV2.y * v0v1.y - deltaUV1.y * v0v2.y),
						k * (deltaUV2.y * v0v1.z - deltaUV1.y * v0v2.z));

					v0.tangent = tangent;
					v1.tangent = tangent;
					v2.tangent = tangent;
				}
			}
		}

		modelMetadata.vertexCount = vertices.size();
		modelMetadata.indexCount = indices.size();

		assets->write((char *)&modelMetadata, sizeof(ModelMetadata));
		assets->write((char *)vertices.data(),
					  modelMetadata.vertexCount * sizeof(Vertex));
		assets->write((char *)indices.data(),
					  modelMetadata.indexCount * sizeof(uint32_t));

		content.size = sizeof(ModelMetadata) +
					   modelMetadata.vertexCount * sizeof(Vertex) +
					   modelMetadata.indexCount * sizeof(uint32_t);
		return content.offset + content.size;
	} else {
		std::cerr << "Asset file " << path << " does not exist." << std::endl;
	}
	return -1;
}

uintptr_t Writer::writeAsset(Entry &content, std::string path) {
	assets->seekp(content.offset, std::ios_base::beg);
	std::string ext = getExtension(path);
	AssetType type = extensionsToType[ext];

	if (type == AssetType::IMAGE) {
		return writeImage(content, path);
	} else if (type == AssetType::MODEL) {
		return writeModel(content, path);
	} else if (type == AssetType::VOXEL_MODEL) {
		return writeVoxelModel(content, path);
	}
	return -1;
}

Writer::~Writer() { assets->close(); };
