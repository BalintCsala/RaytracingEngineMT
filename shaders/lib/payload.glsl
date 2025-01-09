#include "material.glsl"

struct Payload {
    uint objectId;
    float dist;
    vec3 normal;
    mat3 tbn;
    Material material;
};