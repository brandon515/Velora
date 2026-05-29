#version 450
#pragma shader_stage(fragment)

const uint VELORA_MAX_MATERIALS = 256;
const uint VELORA_TEXURES_PER_MATERIAL = 1;
const uint VELORA_MAX_TEXTURES = VELORA_MAX_MATERIALS * VELORA_TEXURES_PER_MATERIAL;

layout(binding = 1) uniform sampler2D texSampler[VELORA_MAX_TEXTURES];

layout(location = 0) in vec2 texCoordOut;
layout(location = 1) out uint texIndex;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler[texIndex], texCoordOut);
}