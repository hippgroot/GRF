#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 0) uniform texture2D _Textures[];
layout(binding = 2) uniform sampler _Samplers[];

layout(push_constant) uniform PC {
layout(offset = 8)
  uint          texIndex;
  uint          samplerIndex;
} _Constants;

layout(location = 0) in vec2 _UV;

layout(location = 0) out vec4 _FragColor;

void main() {
  vec3 albedo = texture(
    sampler2D(_Textures[_Constants.texIndex], _Samplers[_Constants.samplerIndex]),
    _UV
  ).rgb;

  _FragColor = vec4(albedo, 1.0);
}
