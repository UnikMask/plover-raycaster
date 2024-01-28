#version 460 core

layout (binding=0) uniform RaycasterUniform {
    vec3 cameraPos;
    vec3 cameraDir;
    vec3 cameraUp;
    float fov;
    float aspectRatio;
    float zNear;
    float zFar;
} uRay;

layout (location = 0) in vec2 inPosition;

layout (location = 0) out RayInfo {
    vec3 position;
    vec3 dir;
}

// Get ray information (starting position, direction) from camera info.
void main() {
    vec2 plane = vec2(tan(uRay.fov) * uRay.zNear, tan(uRay.fov) * uRay.zNear / uRay.aspectRatio);
    vec3 cameraRight = cross(uRay.cameraDir, uRay.cameraUp);
    RayInfo.dir = uRay.cameraDir + plane.x * uRay.cameraLeft + plane.y * uRay.cameraUp;
    RayInfo.position = uRay.cameraPosition + zNear * RayInfo.dir;
    RayInfo.dir = normalize(RayInfo.dir);
}
