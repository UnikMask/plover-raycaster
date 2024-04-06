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
ivec3 mapPos;
vec3 deltaDist = 1 / abs(iRay.dir);
vec3 sideDist;
ivec3 tstep;

// OOB pre-compute
vec3 gt0, oobUpper, oobLower; 
vec3 ibThresh;

void onHit(float dist, vec4 tile) {
    outColor = tile * (48 - dist) / 48;
    gl_FragDepth = dist / (iRay.zFar - iRay.zNear);
}

void emptyColor() {
    outColor = vec4(0);
    gl_FragDepth = 1.0f;
}

bool oob() {
    return clamp(mapPos, oobLower, oobUpper) != mapPos;
}

bool bounds_x(vec3 coords) {
    return coords.x >= 0 && coords.x <= bounds.x;
}

bool bounds_y(vec3 coords) {
    return coords.y >= 0 && coords.y <= bounds.y;
}

bool bounds_z(vec3 coords) {
    return coords.z >= 0 && coords.z <= bounds.z;
}

void main() {
    gt0 = vec3(float(iRay.dir.x >= 0), float(iRay.dir.y >= 0), float(iRay.dir.z >= 0));
    oobUpper = gt0 * bounds + (1 - gt0) * iRay.zFar;
    oobLower = -gt0 * iRay.zFar;

    if (oob()) {
        emptyColor();
        return;
    }

    // Get offset from bound to model
    float offset = iRay.zFar;
    vec3 lambda = ((1 - gt0) * bounds - iRay.position) / iRay.dir;
    vec3 coords = iRay.position + lambda.x * iRay.dir;
    if (bounds_y(coords) && bounds_z(coords)) {
        offset = min(lambda.x, offset);
    } 
    coords = iRay.position + lambda.y * iRay.dir;
    if (bounds_x(coords) && bounds_z(coords)) {
        offset = min(lambda.y, offset);
    } 
    coords = iRay.position + lambda.z * iRay.dir;
    if (bounds_x(coords) && bounds_y(coords)) {
        offset = min(lambda.z, offset);
    }
    offset = max(0, offset);

    // Get max distance
    lambda = (gt0 * bounds - iRay.position) / iRay.dir;
    float maxDist = min(iRay.zFar - iRay.zNear, 
                        min(lambda.x, min(lambda.y, lambda.z)));

    vec3 position = iRay.position + offset * iRay.dir;
    mapPos = ivec3(position);
    sideDist = mapPos + vec3(1, 1, 1) - position;
    tstep = ivec3(1, 1, 1);
    if (iRay.dir.x < 0) {
        tstep.x = -1;
        sideDist.x = position.x - mapPos.x;
    }
    if (iRay.dir.y < 0) {
        tstep.y = -1;
        sideDist.y = position.y - mapPos.y;
    }
    if (iRay.dir.z < 0) {
        tstep.z = -1;
        sideDist.z = position.z - mapPos.z;
    }
    sideDist = sideDist * deltaDist + offset;

    // Raycasting loop - increment dist until reach
    float dist = offset;
    vec4 tile = texelFetch(map, mapPos.xzy, 0);
    while (tile.a == 0 && dist <= maxDist) {
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
