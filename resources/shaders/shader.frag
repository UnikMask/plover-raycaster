#version 460 core

layout(set = 0, binding = 0) uniform GlobalUniform {
	mat4 camera;
	vec3 cameraPos;
} global;

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D nrmSampler;

layout(location = 0) in FragIn {
	mat3 TBN;
	vec2 texCoord;
	vec3 fragPos;
} fragIn;

layout(location = 0) out vec4 outColor;

float ambientIntensity = 0.05f;
float diffuseStrength = 0.7f;
float specularStrength = 0.3f;
vec3 lightDir = normalize(vec3(1, 1, -1));

void main()
{
	vec4 nrmSample = texture(nrmSampler, fragIn.texCoord);
	float spec = nrmSample.a;
	vec3 normal = nrmSample.rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(fragIn.TBN * normal);
	float diffuseIntensity = max(dot(normal, lightDir), 0.f) * diffuseStrength;

	vec3 viewDir = normalize(global.cameraPos - fragIn.fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	float specularIntensity = pow(max(dot(viewDir, reflectDir), 0.0), 20) * specularStrength * spec;

	float lightIntensity = (ambientIntensity + diffuseIntensity + specularIntensity);

	outColor = vec4(lightIntensity * texture(texSampler, fragIn.texCoord).rgb, 1.0f);
}
