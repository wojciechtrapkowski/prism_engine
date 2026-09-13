#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

struct MeshInfo
{
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
    int      textureIndex;
    uint     _pad[3];
};

// Won't there be issues with alignment?
struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 textureUV;
};

struct Index
{
    uint idx;
};

layout(buffer_reference, scalar) readonly buffer VertexData
{
    Vertex v[];
};
layout(buffer_reference, scalar) readonly buffer IndexData
{
    Index i[];
};

layout(location = 0) rayPayloadInEXT vec3 payload;

layout(set = 0, binding = 1) uniform sampler2D textures[];
layout(set = 0, binding = 2) readonly buffer MeshInfoTable
{
    MeshInfo meshes[];
};

// barycentric coordinates of the hit point (u, v) — provided by the hardware
hitAttributeEXT vec2 attribs;

void main()
{
    const vec3 bary = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    MeshInfo   mesh     = meshes[gl_InstanceCustomIndexEXT];
    VertexData vertices = VertexData(mesh.vertexBufferAddress);
    IndexData  indices  = IndexData(mesh.indexBufferAddress);

    uint i0 = indices.i[3 * gl_PrimitiveID + 0].idx;
    uint i1 = indices.i[3 * gl_PrimitiveID + 1].idx;
    uint i2 = indices.i[3 * gl_PrimitiveID + 2].idx;

    Vertex v0 = vertices.v[i0];
    Vertex v1 = vertices.v[i1];
    Vertex v2 = vertices.v[i2];

    vec2 uv = v0.textureUV * bary.x + v1.textureUV * bary.y + v2.textureUV * bary.z;

    vec3 color;

    int textureIndex = mesh.textureIndex;
    if (textureIndex == -1) {
        color = vec3(1.0, 0.0, 1.0); // magenta for missing texture
    } else {
        color = texture(textures[nonuniformEXT(textureIndex)], uv).rgb;
    }

    payload = color;
}
