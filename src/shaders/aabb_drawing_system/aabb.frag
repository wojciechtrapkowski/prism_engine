#version 450

layout (location = 0) in flat uint entityId;

layout(location = 0) out vec4 FragColor;

void main() {
    float r = float((entityId * 37u) % 255u) / 255.0;
    float g = float((entityId * 73u) % 255u) / 255.0;
    float b = float((entityId * 113u) % 255u) / 255.0;

    FragColor = vec4(r, g, b, 0.3);
}