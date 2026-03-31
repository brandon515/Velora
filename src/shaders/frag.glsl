#version 450
#pragma shader_stage(fragment)

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 texCoordOut;

layout(location = 0) out vec4 outColor;

layout( push_constant ) uniform constants
{
  uint textureIndex;
  mat4 mvpMatrix;
} PushConstants;

void main() {
    outColor = texture(texSampler, texCoordOut);
}