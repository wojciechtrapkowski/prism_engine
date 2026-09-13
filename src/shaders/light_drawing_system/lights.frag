#version 450

#extension GL_GOOGLE_include_directive : require

#include "push_constants.glsl"

#include "../common_uniforms.glsl"

layout(set = 0, binding = 0) uniform UBO
{
    CommonUniforms commonUniforms;
};

layout(push_constant) uniform PushConstants
{
    FragmentShaderPushConstants pushConstants;
};

layout(location = 0) out vec4 FragColor;
// layout(depth_greater / depth_less) out float gl_FragDepth;

void main()
{
    vec3  lightPosition = pushConstants.position;
    float strength      = pushConstants.strength;

    // We could move this calculation to the CPU side.
    vec4 lightPositionInClipSpace = commonUniforms.projection * commonUniforms.view * vec4(lightPosition, 1.0);

    // It is behind the camera.
    if (lightPositionInClipSpace.w <= 0.0) {
        discard;
    }

    float distanceFromCamera              = lightPositionInClipSpace.w; // larger w = more distant
    float strengthBasedOnDistanceToCamera = strength / sqrt(distanceFromCamera);
    float strengthRadius                  = strengthBasedOnDistanceToCamera * strengthBasedOnDistanceToCamera;

    vec3 lightPositionInNDC = (lightPositionInClipSpace.xyz / lightPositionInClipSpace.w);
    gl_FragDepth            = lightPositionInNDC.z;

    // NDC -1, 1
    // Transform it to 0, 1
    lightPositionInNDC = (lightPositionInNDC * 0.5 + 0.5);

    vec2 lightPositionInScreenSpace = lightPositionInNDC.xy * vec2(commonUniforms.screenResolution);

    const vec3 lightColor = vec3(1.0, 1.0, 1.0);

    vec2  delta        = gl_FragCoord.xy - lightPositionInScreenSpace;
    float deltaSquared = dot(delta, delta);

    if (deltaSquared <= strengthRadius) {
        FragColor = vec4(lightColor, 1.0);
    } else {
        discard;
    }
}