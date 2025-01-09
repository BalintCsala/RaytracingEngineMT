#ifndef OBJDESC_GLSL
#define OBJDESC_GLSL

#include "buffers.glsl"

struct ObjDesc {
    mat4 modelMatrix;
    uint triangleCount;
    int baseColorId;
    int normalId;
    int metallicRoughnessId;
    int emissiveId;
    Indices indices;
    PositionData positions;
    VertexData vertices;
};

struct HitData {
    vec2 uv;
    mat3 tbn;
};

vec2 getUV(ObjDesc desc, uvec3 index, vec3 baryCoords) {
    vec2 uv0 = desc.vertices.v[index.x].uv;
    vec2 uv1 = desc.vertices.v[index.y].uv;
    vec2 uv2 = desc.vertices.v[index.z].uv;
    return baryCoords.x * uv0 + baryCoords.y * uv1 + baryCoords.z * uv2;
}

HitData getHitData(ObjDesc desc, uvec3 index, vec3 baryCoords) {
    vec3 vertPos0 = desc.positions.p[index.x];
    vec3 vertPos1 = desc.positions.p[index.y];
    vec3 vertPos2 = desc.positions.p[index.z];

    vec3 edge1 = vertPos1 - vertPos0;
    vec3 edge2 = vertPos2 - vertPos0;

    vec2 uv = getUV(desc, index, baryCoords);

    vec3 normal0 = desc.vertices.v[index.x].normal;
    vec3 normal1 = desc.vertices.v[index.y].normal;
    vec3 normal2 = desc.vertices.v[index.z].normal;
    vec3 normal = normal0 * baryCoords.x + normal1 * baryCoords.y + normal2 * baryCoords.z;
    float normalLen = length(normal);
    if (normalLen < 0.01) {
        normal = normalize(cross(edge1, edge2));
    } else {
        normal /= normalLen;
    }

    vec3 tangent0 = desc.vertices.v[index.x].tangent;
    vec3 tangent1 = desc.vertices.v[index.y].tangent;
    vec3 tangent2 = desc.vertices.v[index.z].tangent;
    vec3 tangent = tangent0 * baryCoords.x + tangent1 * baryCoords.y + tangent2 * baryCoords.z;
    float tangentLen = length(tangent);
    if (tangentLen < 0.01) {
        vec2 uv0 = desc.vertices.v[index.x].uv;
        vec2 uv1 = desc.vertices.v[index.y].uv;
        vec2 uv2 = desc.vertices.v[index.z].uv;
        vec2 deltaUV1 = uv1 - uv0;
        vec2 deltaUV2 = uv2 - uv0;
        float scaleFactor = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        tangent = normalize(scaleFactor * (deltaUV2.y * edge1 - deltaUV1.y * edge2));
    } else {
        tangent /= tangentLen;
    }

    vec3 bitangent = normalize(cross(tangent, normal));

    return HitData(
        uv,
        mat3(tangent, bitangent, normal)
    );
}



#endif