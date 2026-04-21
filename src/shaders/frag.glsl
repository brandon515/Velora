#version 450
#pragma shader_stage(fragment)

const uint VELORA_MAX_OBJECTS = 256;
const uint VELORA_MAX_MATERIALS = 256;
const uint VELORA_TEXURES_PER_MATERIAL = 1;
const uint VELORA_MAX_TEXTURES = VELORA_MAX_MATERIALS * VELORA_TEXURES_PER_MATERIAL;


layout(binding = 0) uniform UniformBufferObject {
    mat4 mvpMat;
    uint textureIndex;
    uint padding1;
    uint padding2;
    uint padding3;
} ubo[VELORA_MAX_OBJECTS];

layout(binding = 1) uniform sampler2D texSampler[VELORA_MAX_TEXTURES];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 texCoordOut;

layout(location = 0) out vec4 outColor;

layout( push_constant ) uniform constants
{
  uint uboIndex;
};

void main() {
    outColor = texture(texSampler[ubo[uboIndex].textureIndex], texCoordOut);
}