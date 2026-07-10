#version 460

#define PI 3.1415926538f

layout(location = 0) out vec3 _Albedo;

void main() {
  const vec2 positions[3] = vec2[](
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5),
    vec2(0.0, -0.5)
  );

  const vec2 uvs[3] = vec2[](
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(0.5, 0.0)
  );

  vec2 pos = positions[gl_VertexIndex];
  vec2 uv = uvs[gl_VertexIndex];

  float theta = uv.x * PI;
  float ct = cos(theta);
  float st = sin(theta);

  float phi = uv.y * 2.0 * PI;
  float cp = cos(phi);
  float sp = sin(phi);

  vec3 albedo = 0.5 * (vec3(cp * st, sp * st, ct) + 1.0);

  gl_Position = vec4(pos, 0.0, 1.0);
  _Albedo = albedo;
}
