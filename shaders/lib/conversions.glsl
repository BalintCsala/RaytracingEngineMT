#ifndef CONVERSIONS_GLSL
#define CONVERSIONS_GLSL

vec3 screenToView(mat4 projInv, vec2 uv, float depth) {
    vec4 screen = vec4(uv, depth, 1.0) * 2.0 - 1.0;
    vec4 tmp = projInv * screen;
    return tmp.xyz / tmp.w;
}

#endif