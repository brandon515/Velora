#version 450
#pragma shader_stage(vertex)

const uint VELORA_MAX_OBJECTS = 256;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvpMat;
    uint textureIndex;
} ubo[VELORA_MAX_OBJECTS];

layout( push_constant, std430) uniform constants
{
  uint uboIndex;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec2 texCoordOut;

void main() {
    gl_Position = ubo[uboIndex].mvpMat * vec4(inPosition, 1.0);
    texCoordOut = texCoord;
}