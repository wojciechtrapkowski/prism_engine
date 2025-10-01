#version 450

layout(set = 0, binding = 0) uniform CommonUniforms {
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
} commonUniforms;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureUV;

layout(location = 0) out vec3 outPosition;

layout(location = 1) out vec3 outNormal;

void main() {
    gl_Position = commonUniforms.projection * commonUniforms.view * pushConstants.model * vec4(inPosition, 1.0);
    outNormal = normalize(mat3(transpose(inverse(pushConstants.model))) * inNormal);
    outPosition = vec3(pushConstants.model * vec4(inPosition, 1.0));
}