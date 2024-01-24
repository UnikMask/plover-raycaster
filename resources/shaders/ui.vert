#version 460 core

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out uint fragCharacter;

struct UIQuad {
	uint character;
	vec2 pos;
	vec2 size;
	vec4 color;
};

layout(std140,set = 0, binding = 1) readonly buffer QuadBuffer {
	UIQuad quads[];
} quadBuffer;

void main()
{
	UIQuad quad = quadBuffer.quads[gl_VertexIndex / 6];
	uint vertexOffset = gl_VertexIndex % 6;
	float texCoord_x = 0.0;
	float texCoord_y = 0.0;
	if (vertexOffset == 2 ||
		vertexOffset == 1 ||
		vertexOffset == 5) {
		texCoord_x = 1.0;
	}
	if (vertexOffset == 1 ||
		vertexOffset == 5 ||
		vertexOffset == 4) {
		texCoord_y = 1.0;
	}
	vec2 texCoord = vec2(texCoord_x, texCoord_y);
	gl_Position = vec4(quad.pos + quad.size * texCoord, 0.0, 1.0);
	fragTexCoord = texCoord;
	fragColor = quad.color;
	fragCharacter = quad.character;
}
