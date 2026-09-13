struct VertexShaderPushConstants
{
    mat4 model;
    int  textureId;
};
#define VERTEX_PUSH_CONSTANTS_SIZE 68

struct FragmentShaderPushConstants
{
    int lightsCount;
};