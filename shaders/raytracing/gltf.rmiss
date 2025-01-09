#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "../lib/payload.glsl"

layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
    payload.dist = -1.0;
}