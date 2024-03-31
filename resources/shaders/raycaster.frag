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

void onHit(float dist, vec4 tile) {
    outColor = tile * (24 - dist) / 24;
    gl_FragDepth = dist / (iRay.zFar - iRay.zNear);
}

bool oob(vec3 coords, vec3 dir) {
    vec3 bounds = textureSize(map, 0);
    if ((dir.x > 0 && coords.x > bounds.x) || (dir.x < 0 && coords.x < 0)) {
        return true;
    }
    if ((dir.y > 0 && coords.y > bounds.y) || (dir.y < 0 && coords.y < 0)) {
        return true;
    }
    if ((dir.z > 0 && coords.z > bounds.z) || (dir.z < 0 && coords.z < 0)) {
        return true;
    }
    return false;
}

void main() {
    // Set init variable for raycasting loop
    ivec3 mapPos = ivec3(iRay.position);
    vec3 deltaDist = 1 / abs(iRay.dir);
    vec3 sideDist = (mapPos + vec3(1, 1, 1) - iRay.position) * deltaDist;
    ivec3 tstep = ivec3(1, 1, 1);
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
    bool hit = false;
    float dist = min(sideDist.x - deltaDist.x, min(sideDist.y - deltaDist.y, sideDist.z - deltaDist.z));
    vec4 tile = vec4(0, 0, 0, 1);
    while (!hit && dist <= iRay.zFar - iRay.zNear && !oob(mapPos.xzy, iRay.dir)) {
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            dist = sideDist.x - deltaDist.x;
            sideDist.x += deltaDist.x;
            mapPos.x += tstep.x;
        } else if (sideDist.z < sideDist.x && sideDist.z < sideDist.y) {
            dist = sideDist.z - deltaDist.z;
            sideDist.z += deltaDist.z;
            mapPos.z += tstep.z;
        } else {
            dist = sideDist.y - deltaDist.y;
            sideDist.y += deltaDist.y;
            mapPos.y += tstep.y;
        }
        tile = texelFetch(map, mapPos.xzy, 0);
        if (tile != vec4(0, 0, 0, 0)) {
            hit = true;
        }
    }
    if (oob(mapPos.xzy, iRay.dir)) {
        gl_FragDepth = 1;
        return;
    }
    onHit(dist, tile);
}
