#ifndef UNIFORM_BINDINGS_GLSL
#define UNIFORM_BINDINGS_GLSL

#include "uniforms.glsl"

layout(set = 0, binding = 0) uniform _Uniforms {
    Uniforms uni;
};

layout(set = 0, binding = 1) uniform _PrevUniforms {
    Uniforms prev;
};

#endif