#ifndef UNIFORMS_GLSL
#define UNIFORMS_GLSL

struct Uniforms {
    mat4 proj;
    mat4 projInverse;
    mat4 view;
    mat4 viewInverse;
    uint frame;
};

#endif
