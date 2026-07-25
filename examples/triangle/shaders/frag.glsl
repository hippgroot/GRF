#version 460

layout(location = 0) in vec3 _Albedo;
layout(location = 0) out vec4 _FragColor;

void main() {
  _FragColor = vec4(_Albedo, 1.0);
}
