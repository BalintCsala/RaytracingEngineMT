#ifndef SPHERICAL_HARMONICS_GLSL
#define SPHERICAL_HARMONICS_GLSL

#include "constants.glsl"

const int SH_COEFF_COUNT = 9;

struct SH {
    vec3 coeffs[SH_COEFF_COUNT];
};

float evaluateSHCoefficient(int index, vec3 direction) {
    switch (index) {
        case 0:
            return 0.5 * sqrt(1.0 / PI);
        case 1:
            return 0.5 * sqrt(3.0 / PI) * direction.y;
        case 2:
            return 0.5 * sqrt(3.0 / PI) * direction.z;
        case 3:
            return 0.5 * sqrt(3.0 / PI) * direction.x;
        case 4:
            return 0.5 * sqrt(15.0 / PI) * direction.x * direction.y;
        case 5:
            return 0.5 * sqrt(15.0 / PI) * direction.y * direction.z;
        case 6:
            return 0.25 * sqrt(5.0 / PI) * (3.0 * direction.z * direction.z - 1.0);
        case 7:
            return 0.5 * sqrt(15.0 / PI) * direction.x * direction.z;
        case 8:
            return 0.25 * sqrt(15.0 / PI) * (direction.x * direction.x - direction.y * direction.y);
    }
    return 0.0;
}

SH encodeSH(vec3 value, vec3 direction) {
    SH sh;
    for (int i = 0; i < SH_COEFF_COUNT; i++) {
        sh.coeffs[i] = value * evaluateSHCoefficient(i, direction);
    }
    return sh;
}

vec3 decodeSH(SH sh, vec3 direction) {
    vec3 value = vec3(0.0);
    for (int i = 0; i < SH_COEFF_COUNT; i++) {
        value += sh.coeffs[i] * evaluateSHCoefficient(i, direction);
    }
    return value;
}

#endif // SPHERICAL_HARMONICS_GLSL