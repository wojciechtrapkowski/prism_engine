#version 450

#extension GL_GOOGLE_include_directive : require

#include "push_constants.glsl"

#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform CommonUniforms
{
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
}
commonUniforms;

layout(set = 0, binding = 1) uniform sampler2D textures[];

struct LightEntry
{
    vec3  position;
    float strength;
};

layout(set = 0, binding = 2) readonly buffer Lights
{
    LightEntry lights[];
};

layout(push_constant) uniform PushConstants
{
    layout(offset = VERTEX_PUSH_CONSTANTS_SIZE) FragmentShaderPushConstants pushConstants;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTextureUV;
layout(location = 3) in flat int inTextureId;

layout(location = 0) out vec4 FragColor;

void main()
{
    const vec3  lightColor      = vec3(1.0, 1.0, 1.0);
    const float ambientStrength = 0.3;

    vec3 normal = normalize(inNormal);

    vec3 diffuse = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < pushConstants.lightsCount; i++) {
        LightEntry light = lights[i];

        vec3 thisLightDir = light.position - inPosition;

        float multiplier = light.strength / max(length(thisLightDir), 0.01); // to avoid division by zero
        thisLightDir     = normalize(thisLightDir);

        vec3 thisLightDiffuse = max(dot(normal, thisLightDir), 0.0) * lightColor;

        diffuse += multiplier * thisLightDiffuse;
    }

    vec3 ambient = ambientStrength * lightColor;

    vec4 objectColor = vec4(1.0, 0.0, 0.0, 1.0);

    if (inTextureId != -1) {
        objectColor = texture(textures[nonuniformEXT(inTextureId)], inTextureUV);
    }
    vec3 result = (ambient + diffuse) * vec3(objectColor);

    FragColor = vec4(result, 1.0);
}