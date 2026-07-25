#version 460

layout(location = 0) out vec3 _Color;

const vec2 _Positions[3] = vec2[](
  vec2(-0.5f, 0.5f),
  vec2(0.5f, 0.5f),
  vec2(0.0f, -0.5f)
);

const vec3 _Colors[3] = vec3[](
  vec3(1.0f, 0.0f, 0.0f),
  vec3(0.0f, 1.0f, 0.0f),
  vec3(0.0f, 0.0f, 1.0f)
);

void main() {
  vec2 pos = _Positions[gl_VertexIndex];

  gl_Position = vec4(pos, 0.0, 1.0);
  _Color = _Colors[gl_VertexIndex];
}
