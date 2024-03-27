// vim:ft=glsl
#version 460 core

layout (binding = 1) uniform sampler2D map;

layout (location = 0) in RayInfo {
    vec3 position;
    vec3 dir;
    float zNear;
    float zFar;
} iRay;

layout (location = 0) out vec4 outColor;
// layout(depth_any) out float gl_FragDepth;

vec3 tileColors[4] = vec3[](
    vec3(1, 0, 0),
    vec3(0.7, 0, 0),
    vec3(0.4, 0.4, 0.4),
    vec3(0.2, 0.2, 0.2)
);

float getDist(int side, vec3 sideDist, vec3 deltaDist) {
    if (side == 0) {
        return sideDist.x - deltaDist.x;
    } else if (side == 1) {
        return sideDist.z - deltaDist.z;
    } 
    return sideDist.y - deltaDist.y;
}

void main() {
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

    bool hit = false;
    int side = 0;
    vec4 tile = vec4(0, 0, 0, 1);
    while (!hit) {
        if (getDist(side, sideDist, deltaDist) > iRay.zFar - iRay.zNear) {
            hit = true;
            break;
        }
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            sideDist.x += deltaDist.x;
            mapPos.x += tstep.x;
            side = 0;
        } else if (sideDist.z < sideDist.x && sideDist.z < sideDist.y) {
            sideDist.z += deltaDist.z;
            mapPos.z += tstep.z;
            side = 1;
        } else {
            sideDist.y += deltaDist.y;
            mapPos.y += tstep.y;
            side = tstep.y == 1? 3: 2;
            hit = true;
        }
        tile = texelFetch(map, mapPos.xz, 0);
        if (tile.rgb == vec3(0, 0, 0)) {
            hit = true;
        }
    }
    float dist = getDist(side, sideDist, deltaDist);

    outColor = vec4(tileColors[side], 1);
    gl_FragDepth = dist / (iRay.zFar - iRay.zNear);
}
