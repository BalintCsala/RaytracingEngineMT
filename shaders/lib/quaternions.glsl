#if !defined(QUATERNIONS_GLSL)
#define QUATERNIONS_GLSL

#include "constants.glsl"

vec4 quaternionGetRotationToZAxis(vec3 vec) {
	if (vec.z < -0.99999) 
        return vec4(1.0, 0.0, 0.0, 0.0);
	return normalize(vec4(vec.y, -vec.x, 0.0, 1.0 + vec.z));
}


vec4 quaternionMultiply(vec4 a, vec4 b) {
    return vec4(
        a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz),
        a.w * b.w - dot(a.xyz, b.xyz)
    );
}

vec4 quaternionInverse(vec4 q) {
    return vec4(-q.xyz, q.w);
}

vec3 quaternionRotate(vec4 q, vec3 p) {
    vec4 inv = quaternionInverse(q);
    vec4 temp = quaternionMultiply(q, vec4(p, 0.0));
    return quaternionMultiply(temp, inv).xyz;
    return inv.w * temp.xyz + temp.w * inv.xyz + cross(temp.xyz, inv.xyz);
}

vec4 quaternionAxisAngle(vec3 axis, float angle) {
    return vec4(axis * sin(angle / 2.0), cos(angle / 2.0));
}

vec4 quaternionRotateTo(vec3 from, vec3 to) {
    if (dot(from, to) > 1.0 - EPSILON) 
        return vec4(0, 0, 0, 1);
    
    vec3 halfway = normalize(from + to);
    return vec4(
        cross(from, halfway),
        dot(from, halfway)
    );
}

#endif // QUATERNIONS_GLSL