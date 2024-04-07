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
    float zNear;
    float zFar;
} oRay;

// Get ray information (starting position, direction) from camera info.
void main() {
    gl_Position = vec4(inPosition, 0, 1);
    oRay.dir = normalize(uRay.cameraDir 
        - inDisplay.x * tan(uRay.fov / 2) *  uRay.cameraLeft * uRay.aspectRatio 
        + inDisplay.y * uRay.cameraUp * tan(uRay.fov / 2));

    oRay.position = uRay.cameraPos + uRay.zNear * oRay.dir;
    oRay.zNear = uRay.zNear;
    oRay.zFar = uRay.zFar;
}
