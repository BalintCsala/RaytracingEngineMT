#if !defined(BRDF_GLSL)
#define BRDF_GLSL

#include "quaternions.glsl"
#include "constants.glsl"
#include "material.glsl"
#include "rand.glsl"
#include "utils.glsl"

struct BRDFSample {
    bool valid;
    vec3 direction;
    vec3 brdf;
};

BRDFSample invalidSample() {
    BRDFSample samp;
    samp.valid = false;
    return samp;
}

vec3 fresnelSchlick(vec3 F0, float cosTheta) {
    return F0 + (1.0 - F0) * pow((1.0 - cosTheta), 5.0);
}

float distributionGGX(Material material, vec3 v) {
    float alpha2 = material.roughness * material.roughness;
    return 1.0 / (PI * alpha2 * pow(v.x * v.x / alpha2 + v.y * v.y / alpha2 + v.z * v.z , 2.0));
}

float lambdaSmith(Material material, vec3 v) {
    return (-1.0 + sqrt(1.0 + (material.roughness * material.roughness * (v.x * v.x + v.y * v.y)) / v.z / v.z)) / 2.0;
}

float shadowingSmith(Material material, vec3 v) {
    return 1.0 / max(1.0 + lambdaSmith(material, v), EPSILON);
}

float geometrySmith(Material material, vec3 view, vec3 light) {
    return shadowingSmith(material, view) * shadowingSmith(material, light);
}

vec3 brdfDirect(Material material, vec3 normal, vec3 view, vec3 light) {
    vec4 q = quaternionGetRotationToZAxis(normal);
    vec3 V = quaternionRotate(q, view);
    vec3 L = quaternionRotate(q, light);
    vec3 H = normalize(V + L);
    
    float HdotV = clamp(dot(H, V), 0.0, 1.0);
    float NdotH = clamp(H.z, 0.0, 1.0);
    float NdotL = clamp(L.z, 0.0, 1.0);
    float NdotV = clamp(V.z, 0.0, 1.0);
    
    vec3 F = fresnelSchlick(getMaterialF0(material), HdotV);
    float D = distributionGGX(material, H);
    float G = geometrySmith(material, V, L);
    
    vec3 specular = F * D * G / max(4.0 * NdotV * NdotL, EPSILON);
    vec3 diffuse = getMaterialAlbedo(material) / PI;
    
    return (diffuse + specular) * NdotL;
}

vec3 sampleUniformHemisphere(inout uint state) {
    vec2 v = randVec2(state);
    float theta = acos(v.x);
    float phi = 2.0 * PI * v.y;
    return vec3(
        cos(phi) * sin(theta),
        sin(phi) * sin(theta),
        cos(theta)
    );
}

vec3 sampleUniformHemisphere(vec2 v) {
    float theta = acos(v.x);
    float phi = 2.0 * PI * v.y;
    return vec3(
        cos(phi) * sin(theta),
        sin(phi) * sin(theta),
        cos(theta)
    );
}

vec3 sampleCosineWeighted(inout uint state, vec3 normal) {
    vec2 v = randVec2(state);
    float angle = 2.0 * PI * v.x;
    float z = 2.0 * v.y - 1.0;

    vec3 directionOffset = vec3(sqrt(1.0 - z * z) * vec2(cos(angle), sin(angle)), z);
    return normalize(normal + directionOffset);
}

vec3 sampleGGXVNDF(inout uint state, Material material, vec3 Ve) {
    vec2 u = randVec2(state);
	vec3 Vh = normalize(vec3(material.roughness * Ve.x, material.roughness * Ve.y, Ve.z));

	float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
	vec3 T1 = lensq > 0.0 ? vec3(-Vh.y, Vh.x, 0.0) * inversesqrt(lensq) : vec3(1.0, 0.0, 0.0);
	vec3 T2 = cross(Vh, T1);

	float r = sqrt(u.x);
	float phi = 2.0 * PI * u.y;
	float t1 = r * cos(phi);
	float t2 = r * sin(phi);
	float s = 0.5 * (1.0 + Vh.z);
	t2 = mix(sqrt(1.0 - t1 * t1), t2, s);

	vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;

	return normalize(vec3(material.roughness * Nh.x, material.roughness * Nh.y, max(0.0, Nh.z)));
}

BRDFSample sampleDiffuse(inout uint state, Material material, vec3 normal) {
    return BRDFSample(
        true,
        sampleCosineWeighted(state, normal),
        getMaterialAlbedo(material)
    );
}

BRDFSample sampleSpecular(inout uint state, Material material, vec3 normal, vec3 view) {
    vec4 q = quaternionGetRotationToZAxis(normal);
    vec3 V = quaternionRotate(q, view);
    vec3 M = sampleGGXVNDF(state, material, V);
    vec3 L = reflect(-V, M);
    if (L.z < 0.0) {
        return invalidSample();
    }
    vec3 F = fresnelSchlick(getMaterialF0(material), max(dot(M, V), 0.01));
    float G2 = geometrySmith(material, V, L);
    float G1 = shadowingSmith(material, V);

    return BRDFSample(
        true,
        quaternionRotate(quaternionInverse(q), L),
        F * G2 / G1
    );
}

float evaluateSpecularProbability(Material material, vec3 view, vec3 normal) {
    float fresnel = clamp(luminance(fresnelSchlick(getMaterialF0(material), max(dot(view, normal), 0.0))), 0.0, 1.0);
    
    float diffuse = luminance(getMaterialAlbedo(material)) * (1.0 - material.metalness) * (1.0 - fresnel);
    float specular = fresnel;
    
    return specular / max(specular + diffuse, EPSILON);
}

#endif // BRDF_GLSL