// vim:ft=glsl
#version 460 core

layout (binding = 1) uniform sampler3D map;

layout (location = 0) in RayInfo {
    vec3 position;
    vec3 dir;
    float zNear;
    float zFar;
} iRay;

layout (location = 0) out vec4 outColor;
layout(depth_greater) out float gl_FragDepth;

vec3 bounds = textureSize(map, 0).xzy;
ivec3 mapPos = ivec3(iRay.position);
vec3 deltaDist = 1 / abs(iRay.dir);
vec3 sideDist;
ivec3 tstep;

void onHit(float dist, vec4 tile) {
    outColor = tile * (48 - dist) / 48;
    gl_FragDepth = dist / (iRay.zFar - iRay.zNear);
}

void emptyColor() {
    outColor = vec4(0);
    gl_FragDepth = 1.0f;
}

bool oob() {
    return (iRay.dir.x >= 0 && mapPos.x > bounds.x) || (iRay.dir.x < 0 && mapPos.x < 0) 
        || (iRay.dir.y >= 0 && mapPos.y > bounds.y) || (iRay.dir.y < 0 && mapPos.y < 0)
        || (iRay.dir.z >= 0 && mapPos.z > bounds.z) || (iRay.dir.z < 0 && mapPos.z < 0);
}

void main() {
    if (oob()) {
        emptyColor();
        return;
    }

    sideDist = (mapPos + vec3(1, 1, 1) - iRay.position) * deltaDist;
    tstep = ivec3(1, 1, 1);
    if (iRay.dir.x < 0) {
        tstep.x = -1;
        sideDist.x = (iRay.position.x - mapPos.x) * deltaDist.x;
    }
    if (iRay.dir.y < 0) {
        tstep.y = -1;
        sideDist.y = (iRay.position.y - mapPos.y) * deltaDist.y;
    }
    if (iRay.dir.z < 0) {
        tstep.z = -1;
        sideDist.z = (iRay.position.z - mapPos.z) * deltaDist.z;
    }

    // Raycasting loop - increment dist until reach
    float dist = 0;
    vec4 tile = vec4(0);
    while (tile.a == 0 && dist <= iRay.zFar - iRay.zNear && !oob()) {
        float minDist = min(sideDist.x, min(sideDist.y, sideDist.z));
        if (minDist == sideDist.x) {
            dist = sideDist.x;
            sideDist.x += deltaDist.x;
            mapPos.x += tstep.x;
        } else if (minDist == sideDist.z) {
            dist = sideDist.z;
            sideDist.z += deltaDist.z;
            mapPos.z += tstep.z;
        } else {
            dist = sideDist.y;
            sideDist.y += deltaDist.y;
            mapPos.y += tstep.y;
        }
        tile = texelFetch(map, mapPos.xzy, 0);
    }
    if (tile.a != 0) {
        onHit(dist, tile);
    } else {
        emptyColor();
    }
}
