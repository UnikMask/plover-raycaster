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
vec3 dir = normalize(iRay.dir);
vec3 bounds = textureSize(map, 0).xzy;
ivec3 mapPos = ivec3(iRay.position);
vec3 deltaDist = 1 / abs(dir);

void onHit(float dist, vec4 tile) {
    vec3 pos = iRay.position + dist * dir;
    vec3 mins = min(ceil(pos) - pos, pos - floor(pos));
    float factor = 1 - (0.7) 
        * float(min(mins.x + mins.y, min(mins.x + mins.z, mins.y + mins.z)) < 0.02);
    outColor = factor * tile * (48 - dist) / 48;
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
    vec3 gt0 = vec3(float(dir.x >= 0), float(dir.y >= 0), float(dir.z >= 0));

    // Return early if ray guaranteed never to hit anything
    vec3 oobUpper = gt0 * bounds + (1 - gt0) * iRay.zFar;
    vec3 oobLower = -gt0 * iRay.zFar;
    if (clamp(mapPos, oobLower, oobUpper) != mapPos) {
        emptyColor();
        return;
    }

    // Get offset from ray original position to model bounds
    float offset = iRay.zFar;
    vec3 lambda = ((1 - gt0) * bounds - iRay.position) / dir;
    if (bounds_yz(iRay.position + lambda.x * dir)) {
        offset = min(lambda.x, offset);
    } 
    if (bounds_xz(iRay.position + lambda.y * dir)) {
        offset = min(lambda.y, offset);
    } 
    if (bounds_xy(iRay.position + lambda.z * dir)) {
        offset = min(lambda.z, offset);
    }
    offset = max(0, offset);

    // Get max distance
    lambda = (gt0 * bounds - iRay.position) / dir;
    float maxDist = min(iRay.zFar, min(lambda.x, min(lambda.y, lambda.z)));

    // Set side distance and step on xyz-axis per loop
    vec3 position = iRay.position + offset * dir;
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
