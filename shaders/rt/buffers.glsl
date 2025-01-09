#ifndef BUFFERS_GLSL
#define BUFFERS_GLSL

#include "vertex.glsl"

layout(buffer_reference, scalar) buffer Indices {
    uvec3 i[];
};

layout(buffer_reference, scalar) buffer VertexData {
    Vertex v[];
};

layout(buffer_reference, scalar) buffer PositionData {
    vec3 p[];
};

#endif