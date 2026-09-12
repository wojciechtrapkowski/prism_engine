#version 450

layout(set = 0, binding = 0) uniform CommonUniforms
{
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
}
commonUniforms;

struct BoxTransformEntry {
    vec4 lower;
    vec3 upper;
    uint entityId;
};

layout (set = 0, binding = 1, std430) readonly buffer BoxTransformsBuffer
{
     BoxTransformEntry boxTransforms[];
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureUV;

layout(location = 0) out flat uint outEntityId;

void main()
{
    BoxTransformEntry entry = boxTransforms[gl_InstanceIndex];

    // Box center is in 0, 0, 0 - that's why we move it by center.
    // Multiply first by scale, so that we are scaling box in the origin.
    vec3 center = (entry.lower.xyz + entry.upper) * 0.5;
    vec3 halfExtent = (entry.upper - entry.lower.xyz) * 0.5;
    vec3 boxPosition = inPosition * halfExtent + center;

    gl_Position  = commonUniforms.projection * commonUniforms.view * vec4(boxPosition, 1.0);
    
    outEntityId = entry.entityId;

}