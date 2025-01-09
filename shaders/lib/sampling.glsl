#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

#define CREATE_BILINEAR_SAMPLER(image)                      \
vec4 image##Sample(vec2 pixel) {                            \
    vec2 frac = fract(pixel);                               \
    ivec2 pix = ivec2(floor(pixel));                        \
    vec4 value00 = imageLoad(image, pix + ivec2(0, 0));     \
    vec4 value10 = imageLoad(image, pix + ivec2(1, 0));     \
    vec4 value01 = imageLoad(image, pix + ivec2(0, 1));     \
    vec4 value11 = imageLoad(image, pix + ivec2(1, 1));     \
    return mix(                                             \
        mix(value00, value10, frac.x),                      \
        mix(value01, value11, frac.x),                      \
        frac.y                                              \
    );                                                      \
}

#endif