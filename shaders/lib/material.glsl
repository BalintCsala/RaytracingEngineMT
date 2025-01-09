#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

struct Material {
    vec3 baseColor;
    float metalness;
    float roughness;
    vec3 emission;
    bool refractive;
};

vec3 getMaterialAlbedo(Material material) {
    return material.baseColor * (1.0 - material.metalness);
}

vec3 getMaterialF0(Material material) {
    return mix(vec3(0.04), material.baseColor, material.metalness);
}

#endif // MATERIAL_GLSL