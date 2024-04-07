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

// Raycasting loop vars
vec3 bounds = textureSize(map, 0).xzy;
ivec3 mapPos = ivec3(iRay.position);
vec3 deltaDist = 1 / abs(iRay.dir);

void onHit(float dist, vec4 tile) {
    outColor = tile * (48 - dist) / 48;
    float temp1 = (iRay.zFar + iRay.zNear) / (iRay.zFar - iRay.zNear);
    float temp2 = -(iRay.zFar * iRay.zNear) / (iRay.zFar - iRay.zNear);
    gl_FragDepth =  (dist * temp1 + temp2) / dist;
}

void emptyColor() {
    outColor = vec4(0);
    gl_FragDepth = 1.0f;
}

bool bounds_yz(vec3 coords) {
    return coords.z >= 0 && coords.z <= bounds.z
    && coords.y >= 0 && coords.y <= bounds.y;
}

bool bounds_xz(vec3 coords) {
    return coords.x >= 0 && coords.x <= bounds.x
    && coords.z >= 0 && coords.z <= bounds.z;
}

bool bounds_xy(vec3 coords) {
    return coords.x >= 0 && coords.x <= bounds.x
    && coords.y >= 0 && coords.y <= bounds.y;
}

void main() {
    vec3 gt0 = vec3(float(iRay.dir.x >= 0), float(iRay.dir.y >= 0), float(iRay.dir.z >= 0));

    // Return early if ray guaranteed never to hit anything
    vec3 oobUpper = gt0 * bounds + (1 - gt0) * iRay.zFar;
    vec3 oobLower = -gt0 * iRay.zFar;
    if (clamp(mapPos, oobLower, oobUpper) != mapPos) {
        emptyColor();
        return;
    }

    // Get offset from ray original position to model bounds
    float offset = iRay.zFar;
    vec3 lambda = ((1 - gt0) * bounds - iRay.position) / iRay.dir;
    if (bounds_yz(iRay.position + lambda.x * iRay.dir)) {
        offset = min(lambda.x, offset);
    } 
    if (bounds_xz(iRay.position + lambda.y * iRay.dir)) {
        offset = min(lambda.y, offset);
    } 
    if (bounds_xy(iRay.position + lambda.z * iRay.dir)) {
        offset = min(lambda.z, offset);
    }
    offset = max(0, offset);

    // Get max distance
    lambda = (gt0 * bounds - iRay.position) / iRay.dir;
    float maxDist = min(iRay.zFar, min(lambda.x, min(lambda.y, lambda.z)));

    // Set side distance and step on xyz-axis per loop
    vec3 position = iRay.position + offset * iRay.dir;
    mapPos = ivec3(position);
    ivec3 tstep = ivec3(2 * gt0 - 1);
    vec3 sideDist = (tstep * (mapPos - position) + gt0) * deltaDist + offset;

    // Raycasting loop - increment dist until reach
    float dist = offset;
    vec4 tile = texelFetch(map, mapPos.xzy, 0);
    while (tile.a == 0 && dist <= maxDist) {
        dist = min(sideDist.x, min(sideDist.y, sideDist.z));
        if (dist == sideDist.x) {
            sideDist.x += deltaDist.x;
            mapPos.x += tstep.x;
        } else if (dist == sideDist.z) {
            sideDist.z += deltaDist.z;
            mapPos.z += tstep.z;
        } else {
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
