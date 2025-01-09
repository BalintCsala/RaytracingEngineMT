#ifndef BINDINGS_GLSL
#define BINDINGS_GLSL

#include "buffers.glsl"
#include "objdesc.glsl"
#include "vertex.glsl"

layout(set = 2, binding = 0) uniform accelerationStructureEXT tlas;

layout(set = 2, binding = 1, scalar) buffer BufferAddresses {
    ObjDesc o[];
} addresses;

layout(set = 2, binding = 2) uniform sampler2D textureSamplers[];

layout(set = 2, binding = 3) buffer EmissiveObjectIds {
    uint ids[];
} emissiveObjects;

#endif