#include "Renderer.h"

#include "Mesh.h"
#include "Texture.h"
#include "UI.h"
#include "VulkanContext.h"
#include "lapwing.h"
#include "raycaster.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <obj/tiny_obj_loader.h>

void Renderer::init() {
	context = new VulkanContext{};
	context->initWindow();
	context->initVulkan();

    VoxelModelMetadata metadata;
    Voxel *data = loader.loadVoxelModel("map.vox", &metadata);
    VoxelMap map = VoxelMap(metadata, data, BitmapFormat::RGBA8);

    Texture lvlTex;
    createTexture(*context, map, lvlTex);

	context->raycasterCtx = new RaycasterContext(lvlTex, context);
}

bool Renderer::render() { return context->render(); }

void Renderer::cleanup() {
	context->cleanup();
	loader.cleanup();
	delete this->context;
}

void Renderer::processCommand(RenderCommand inCmd) {
	u32 cmdID = inCmd.id;

	switch (inCmd.tag) {
	case CREATE_MESH: {
		CreateMeshData data = inCmd.v.createMesh;
		ModelMetadata metadata;

		ModelData modelData = loader.loadModel(data.name, &metadata);

		RenderMessage message{MESH_CREATED, cmdID};
		message.v.meshCreated.meshID =
			createMesh(*context, (Vertex *)modelData.vertices,
					   metadata.vertexCount, (uint32_t *)modelData.indices,
					   metadata.indexCount, data.materialID);
		messageQueue.push(message);
		break;
	}
	case CREATE_MATERIAL: {
		CreateMaterialData materialData = inCmd.v.createMaterial;
		const char *textureName = materialData.textureName;
		const char *normalName = materialData.normalName;

		RenderMessage message{MATERIAL_CREATED, cmdID};
		message.v.materialCreated.materialID =
			createMaterial(*context, loader, textureName, normalName);
		messageQueue.push(message);
		break;
	}
	case SET_MESH_TRANSFORM: {
		SetMeshTransformData meshTransformData = inCmd.v.setMeshTransform;
		Mesh *mesh = context->meshes[meshTransformData.meshID];
		mesh->transform = meshTransformData.transform;
		break;
	}
	case SET_CAMERA: {
		SetCameraData cameraData = inCmd.v.setCamera;
		context->camera = cameraData.camera;
		break;
	}
	case SET_RENDER_MODE: {
		SetRenderModeData renderModeData = inCmd.v.setRenderMode;
		if (renderModeData.mode == RENDER_MODE_WIREFRAME) {
			context->wireframeEnabled = true;
		} else {
			context->wireframeEnabled = false;
		}
		break;
	}
	}
}

void Renderer::processCommands() {
	while (commandQueue.hasMessage()) {
		processCommand(commandQueue.pop());
	}
}

void Renderer::UI_Clear() { context->ui.clear(); }

void Renderer::UI_Rect(Vec4 color, UVec2 pos, UVec2 size) {
	context->ui.writeRect(context, color, pos, size);
}

void Renderer::UI_Text(Vec4 color, UVec2 pos, char *text) {
	context->ui.writeText(context, text, pos, color);
}
