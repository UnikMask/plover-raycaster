// vim:ft=glsl
#version 460 core

layout (std140, binding = 0) uniform RaycasterUniform {
    vec3 cameraPos;
    vec3 cameraDir;
    vec3 cameraUp;
    vec3 cameraLeft;
    float fov;
    float aspectRatio;
    float zNear;
    float zFar;
} uRay;

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inDisplay;

layout (location = 0) out RayInfo {
    vec3 position;
    vec3 dir;
} oRay;

// Get ray information (starting position, direction) from camera info.
void main() {
    gl_Position = vec4(inPosition, 0.1f, 1.0);
    vec2 plane = vec2(tan(uRay.fov) * uRay.zNear, tan(uRay.fov) * uRay.zNear / uRay.aspectRatio);
    oRay.dir = normalize(uRay.cameraDir + inDisplay.x * plane.x * cameraLeft - inDisplay.y * plane.y * uRay.cameraUp);
    oRay.position = uRay.cameraPos + uRay.zNear * oRay.dir;
}
