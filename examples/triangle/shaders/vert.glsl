#version 460
#extension GL_EXT_buffer_reference : require

struct Vertex {
  vec2 pos;
  vec2 uv;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
  Vertex vertices[];
};

layout(push_constant) uniform PC {
  VertexBuffer  vertexBuffer;
  uint          texIndex;
  uint          samplerIndex;
} _Constants;

layout(location = 0) out vec2 _UV;

void main() {
  Vertex vertex = _Constants.vertexBuffer.vertices[gl_VertexIndex];

  gl_Position = vec4(vertex.pos, 0.0f, 1.0f);
  _UV = vertex.uv;
}
