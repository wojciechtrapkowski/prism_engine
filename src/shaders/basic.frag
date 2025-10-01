#version 450

layout(set = 0, binding = 0) uniform CommonUniforms {
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
} commonUniforms;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 FragColor;

void main() {
    const vec3 lightColor = vec3(1.0, 1.0, 1.0);
    const float ambientStrength = 0.2;

    vec3 normal = normalize(inNormal);

    vec3 lightDir = normalize(vec3(commonUniforms.cameraPosition) - inPosition);
    vec3 diffuse = max(dot(normal, lightDir), 0.0) * lightColor;

    vec3 ambient = ambientStrength * lightColor;

    vec4 objectColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    vec3 result = (ambient + diffuse) * vec3(objectColor);

    FragColor = vec4(result, 1.0);
}