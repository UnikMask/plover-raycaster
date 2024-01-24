#version 460 core

layout(set = 0, binding = 0) uniform GlobalUniform {
	mat4 camera;
	vec3 cameraPos;
} global;

layout(set = 2, binding = 0) uniform MeshUniform {
	mat4 model;
} mesh;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out FragIn {
	mat3 TBN;
	vec2 texCoord;
	vec3 fragPos;
} fragIn;

void main()
{
	gl_Position = global.camera * mesh.model * vec4(inPosition, 1.0);
	vec3 T = normalize(vec3(mesh.model * vec4(inTangent, 0.0)));
	vec3 N = normalize(vec3(mesh.model * vec4(inNormal, 0.0)));
	vec3 B = cross(N, T);
	fragIn.TBN = mat3(T, B, N);
	fragIn.texCoord = inTexCoord;
	fragIn.fragPos = vec3(mesh.model * vec4(inPosition, 1.0));
}
