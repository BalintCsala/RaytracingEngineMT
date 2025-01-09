#version 460
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "../rt/bindings.glsl"

#include "../lib/payload.glsl"

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT Payload payload;

void main() {
	payload.dist = gl_HitTEXT;
    ObjDesc desc = addresses.o[gl_InstanceID];

    uvec3 index = desc.indices.i[gl_PrimitiveID];
    vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    HitData hitData = getHitData(desc, index, bary);
    payload.tbn = hitData.tbn;

    if (desc.baseColorId < 0) {
        payload.material.baseColor = vec3(1.0);
        payload.material.refractive = false;
    } else {
        vec4 color = texture(textureSamplers[nonuniformEXT(desc.baseColorId)], hitData.uv);
        payload.material.baseColor = pow(color.rgb, vec3(2.2));
        payload.material.refractive = color.a < 0.99;
    }

    if (desc.normalId < 0) {
        payload.normal = vec3(0, 0, 1);
    } else {
        payload.normal = textureLod(textureSamplers[nonuniformEXT(desc.normalId)], hitData.uv, 0.0).rgb * 2.0 - 1.0;
    }
    payload.normal = normalize(hitData.tbn * payload.normal);

    if (desc.metallicRoughnessId < 0) {
        payload.material.metalness = 0.0;
        payload.material.roughness = 1.0;
    } else {
        vec4 value = texture(textureSamplers[nonuniformEXT(desc.metallicRoughnessId)], hitData.uv);
        payload.material.metalness = value.b;
        payload.material.roughness = clamp(value.g * value.g, 0.01, 1.0);
    }

    if (desc.emissiveId < 0) {
        payload.material.emission = vec3(0.0);
    } else {
        payload.material.emission = texture(textureSamplers[nonuniformEXT(desc.emissiveId)], hitData.uv).rgb * 100.0;
    }
    payload.objectId = gl_InstanceID;
}
