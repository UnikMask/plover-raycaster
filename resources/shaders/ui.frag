#version 460 core
layout(set = 0, binding = 0) uniform sampler2DArray texSampler;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in uint fragCharacter;

layout(location = 0) out vec4 outColor;

void main()
{
	if (fragCharacter == 255) {
		outColor = fragColor;
	}
	else {
		outColor = fragColor * texture(texSampler, vec3(fragTexCoord, fragCharacter));
	}
}
