#version 450
#pragma shader_stage(vertex)

const uint VELORA_MAX_OBJECTS = 256;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvpMat;
    uint textureIndex;
    uint skinOffset;
} ubo[VELORA_MAX_OBJECTS];

layout(binding = 2) uniform SkinMatrix{
  mat4 skinMat
} skinMatrix[VELORA_MAX_JOINTS];

layout( push_constant, std430) uniform constants
{
  uint uboIndex;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 weights;
layout(location = 4) in vec4 joints;

layout(location = 0) out vec2 texCoordOut;
layout(location = 1) out uint texIndex;

void main() {
    gl_Position = ubo[uboIndex].mvpMat * vec4(inPosition, 1.0);
    texCoordOut = texCoord;
    texIndex = ubo[uboIndex].textureIndex;
}