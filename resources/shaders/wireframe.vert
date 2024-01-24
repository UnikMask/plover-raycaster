#version 460 core

layout(set = 0, binding = 0) uniform GlobalUniform {
	mat4 camera;
	vec3 cameraPos;
} global;

layout(set = 1, binding = 0) uniform MeshUniform {
	mat4 model;
} mesh;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

void main()
{
	vec3 throwaway = inNormal + inTangent + vec3(inTexCoord, 1.0);
	gl_Position = global.camera * mesh.model * vec4(inPosition, 1.0);
}
