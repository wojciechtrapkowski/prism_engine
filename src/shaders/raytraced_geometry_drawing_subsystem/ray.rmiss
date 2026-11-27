#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 payload;

void main()
{
    // Simple sky-blue background gradient based on ray direction
    payload = vec3(0.1, 0.2, 0.4);
}
