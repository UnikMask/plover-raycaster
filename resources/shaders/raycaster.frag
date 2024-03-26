// vim:ft=glsl
#version 460 core

layout (location = 0) in RayInfo {
    vec3 position;
    vec3 dir;
} iRay;

layout (location = 0) out vec4 outColor;
// layout(depth_any) out float gl_FragDepth;

// Get hit location from ray information
void main() {
    outColor = vec4(1, 1, 1, 1);
    // gl_FragDepth = 0.99f;
}
