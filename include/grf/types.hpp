#pragma once

#include <cstdint>
#include <cstdlib>
#include <print>
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace grf {

class SwapchainImage;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;

using str = std::string;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using dvec2 = glm::dvec2;
using dvec3 = glm::dvec3;
using dvec4 = glm::dvec4;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using quat = glm::quat;

enum class Format : i32 {
  undefined               = 0,

  r8_unorm                = 9,
  r8_snorm                = 10,
  r8_uint                 = 13,
  r8_sint                 = 14,

  rg8_unorm               = 16,
  rg8_snorm               = 17,
  rg8_uint                = 20,
  rg8_sint                = 21,

  rgba8_unorm             = 37,
  rgba8_snorm             = 38,
  rgba8_uint              = 41,
  rgba8_sint              = 42,
  rgba8_srgb              = 43,

  bgra8_unorm             = 44,
  bgra8_srgb              = 50,

  a2bgr10_unorm_pack32    = 64,

  r16_unorm               = 70,
  r16_uint                = 74,
  r16_sint                = 75,
  r16_sfloat              = 76,

  rg16_unorm              = 77,
  rg16_uint               = 81,
  rg16_sint               = 82,
  rg16_sfloat             = 83,

  rgba16_unorm            = 91,
  rgba16_uint             = 95,
  rgba16_sint             = 96,
  rgba16_sfloat           = 97,

  r32_uint                = 98,
  r32_sint                = 99,
  r32_sfloat              = 100,
  rg32_uint               = 101,
  rg32_sint               = 102,
  rg32_sfloat             = 103,
  rgb32_uint              = 104,
  rgb32_sint              = 105,
  rgb32_sfloat            = 106,
  rgba32_uint             = 107,
  rgba32_sint             = 108,
  rgba32_sfloat           = 109,

  b10g11r11_ufloat_pack32 = 122,

  d16_unorm               = 124,
  d32_sfloat              = 126,
  d24_unorm_s8_uint       = 129,
  d32_sfloat_s8_uint      = 130,

  bc1_rgba_unorm_block    = 133,
  bc1_rgba_srgb_block     = 134,
  bc3_unorm_block         = 137,
  bc3_srgb_block          = 138,
  bc4_unorm_block         = 139,
  bc4_snorm_block         = 140,
  bc5_unorm_block         = 141,
  bc5_snorm_block         = 142,
  bc6h_ufloat_block       = 143,
  bc6h_sfloat_block       = 144,
  bc7_unorm_block         = 145,
  bc7_srgb_block          = 146
};

enum class ColorSpace : i32 {
  srgb_nonlinear          = 0,

  display_p3_nonlinear    = 1000104001,
  extended_srgb_linear    = 1000104002,
  display_p3_linear       = 1000104003,
  hdr10_st2084            = 1000104008,
  hdr10_hlg               = 1000104010,
  extended_srgb_nonlinear = 1000104014
};

enum class Key : u16 {
  Space         = 32,
  Apostrophe    = 39,
  Comma         = 44,
  Minus         = 45,
  Period        = 46,
  Slash         = 47,

  Num0          = 48,
  Num1          = 49,
  Num2          = 50,
  Num3          = 51,
  Num4          = 52,
  Num5          = 53,
  Num6          = 54,
  Num7          = 55,
  Num8          = 56,
  Num9          = 57,

  Semicolon     = 59,
  Equal         = 61,

  A             = 65,
  B             = 66,
  C             = 67,
  D             = 68,
  E             = 69,
  F             = 70,
  G             = 71,
  H             = 72,
  I             = 73,
  J             = 74,
  K             = 75,
  L             = 76,
  M             = 77,
  N             = 78,
  O             = 79,
  P             = 80,
  Q             = 81,
  R             = 82,
  S             = 83,
  T             = 84,
  U             = 85,
  V             = 86,
  W             = 87,
  X             = 88,
  Y             = 89,
  Z             = 90,

  LeftBracket   = 91,
  Backslash     = 92,
  RightBracket  = 93,
  GraveAccent   = 96,

  World1        = 161,
  World2        = 162,

  Escape        = 256,
  Enter         = 257,
  Tab           = 258,
  Backspace     = 259,
  Insert        = 260,
  Delete        = 261,
  Right         = 262,
  Left          = 263,
  Down          = 264,
  Up            = 265,
  PageUp        = 266,
  PageDown      = 267,
  Home          = 268,
  End           = 269,
  CapsLock      = 280,
  ScrollLock    = 281,
  NumLock       = 282,
  PrintScreen   = 283,
  Pause         = 284,

  F1            = 290,
  F2            = 291,
  F3            = 292,
  F4            = 293,
  F5            = 294,
  F6            = 295,
  F7            = 296,
  F8            = 297,
  F9            = 298,
  F10           = 299,
  F11           = 300,
  F12           = 301,
  F13           = 302,
  F14           = 303,
  F15           = 304,
  F16           = 305,
  F17           = 306,
  F18           = 307,
  F19           = 308,
  F20           = 309,
  F21           = 310,
  F22           = 311,
  F23           = 312,
  F24           = 313,
  F25           = 314,

  Kp0           = 320,
  Kp1           = 321,
  Kp2           = 322,
  Kp3           = 323,
  Kp4           = 324,
  Kp5           = 325,
  Kp6           = 326,
  Kp7           = 327,
  Kp8           = 328,
  Kp9           = 329,
  KpDecimal     = 330,
  KpDivide      = 331,
  KpMultiply    = 332,
  KpSubtract    = 333,
  KpAdd         = 334,
  KpEnter       = 335,
  KpEqual       = 336,

  LeftShift     = 340,
  LeftControl   = 341,
  LeftAlt       = 342,
  LeftSuper     = 343,
  RightShift    = 344,
  RightControl  = 345,
  RightAlt      = 346,
  RightSuper    = 347,
  Menu          = 348
};

enum class MouseButton : u8 {
  Left    = 0,
  Right   = 1,
  Middle  = 2,
  Button4 = 3,
  Button5 = 4,
  Button6 = 5,
  Button7 = 6,
  Button8 = 7
};

enum class ShaderStage {
  Vertex    = 1,
  Fragment  = 16,
  Compute   = 32
};

enum class Topology {
  PointList     = 0,
  LineList      = 1,
  LineStrip     = 2,
  TriangleList  = 3,
  TriangleStrip = 4,
  TriangleFan   = 5
};

enum class PolygonMode {
  Fill  = 0,
  Line  = 1,
  Point = 2
};

enum class CullMode {
  None          = 0,
  Front         = 1,
  Back          = 2,
  FrontAndBack  = 3
};

enum class FrontFace {
  CounterClockwise  = 0,
  Clockwise         = 1
};

enum class CompareOp {
  Never           = 0,
  Less            = 1,
  Equal           = 2,
  LessOrEqual     = 3,
  Greater         = 4,
  NotEqual        = 5,
  GreaterOrEqual  = 6,
  Always          = 7
};

enum class BlendFactor {
  Zero                  = 0,
  One                   = 1,
  SrcColor              = 2,
  OneMinusSrcColor      = 3,
  DstColor              = 4,
  OneMinusDstColor      = 5,
  SrcAlpha              = 6,
  OneMinusSrcAlpha      = 7,
  DstAlpha              = 8,
  OneMinusDstAlpha      = 9,
  ConstantColor         = 10,
  OneMinusConstantColor = 11,
  ConstantAlpha         = 12,
  OneMinusConstantAlpha = 13,
  SrcAlphaSaturate      = 14
};

enum class BlendOp {
  Add             = 0,
  Subtract        = 1,
  ReverseSubtract = 2,
  Min             = 3,
  Max             = 4
};

enum class SampleCount {
  e1  = 1,
  e2  = 2,
  e4  = 4,
  e8  = 8,
  e16 = 16
};

enum class Layout {
  Undefined               = 0,
  General                 = 1,
  ColorAttachmentOptimal  = 2,
  ShaderReadOptimal       = 5,
  TransferSrcOptimal      = 6,
  TransferDstOptimal      = 7,
  DepthAttachmentOptimal  = 1000241000,
  PresentSrc              = 1000001002
};

enum class LoadOp {
  Load      = 0,
  Clear     = 1,
  DontCare  = 2
};

enum class StoreOp {
  Store     = 0,
  DontCare  = 1
};

enum class BufferIntent {
  GPUOnly,
  FrequentUpdate,
  Readable
};

enum class IndexType {
  u16,
  u32
};

enum class Filter {
  Nearest = 0,
  Linear  = 1
};

enum class MipmapMode {
  Nearest = 0,
  Linear  = 1
};

enum class AddressMode {
  Repeat            = 0,
  MirroredRepeat    = 1,
  ClampToEdge       = 2,
  ClampToBorder     = 3,
  MirrorClampToEdge = 4
};

enum class ImageUsage : u32 {
  None            = 0u,
  Sampled         = 4u,
  Storage         = 8u,
  ColorAttachment = 16u,
  DepthAttachment = 32u
};

enum class BufferStage : u32 {
  None            = 0u,
  Transfer        = 1u,
  Indirect        = 2u,
  Index           = 4u,
  VertexShader    = 8u,
  FragmentShader  = 16u,
  ComputeShader   = 32u,
  All             = 63u
};

struct BlendState {
  bool        enable          = false;
  BlendFactor srcColorFactor  = BlendFactor::One;
  BlendFactor dstColorFactor  = BlendFactor::Zero;
  BlendOp     colorOp         = BlendOp::Add;
  BlendFactor srcAlphaFactor  = BlendFactor::One;
  BlendFactor dstAlphaFactor  = BlendFactor::Zero;
  BlendOp     alphaOp         = BlendOp::Add;
};

struct GraphicsPipelineSettings {
  std::vector<Format>     colorFormats;
  Format                  depthFormat     = Format::undefined;
  Topology                topology        = Topology::TriangleList;
  PolygonMode             polygonMode     = PolygonMode::Fill;
  CullMode                cullMode        = CullMode::Back;
  FrontFace               frontFace       = FrontFace::CounterClockwise;
  bool                    depthTest       = false;
  bool                    depthWrite      = false;
  CompareOp               depthCompareOp  = CompareOp::Less;
  std::vector<BlendState> blends;
  SampleCount             sampleCount     = SampleCount::e1;
};

struct DrawIndirectCommand {
  u32 vertexCount   = 0;
  u32 instanceCount = 1;
  u32 firstVertex   = 0;
  u32 firstInstance = 0;
};

struct DrawIndexedIndirectCommand {
  u32 indexCount    = 0;
  u32 instanceCount = 1;
  u32 firstIndex    = 0;
  i32 vertexOffset  = 0;
  u32 firstInstance = 0;
};

struct DispatchIndirectCommand {
  u32 x = 1;
  u32 y = 1;
  u32 z = 1;
};

template <typename T>
class Ring {
  std::vector<T>  m_data;
  u32             m_index = 0;

public:
  Ring() = default;
  explicit Ring(std::vector<T> data) : m_data(std::move(data)) { validate(); }
  Ring(u32 count, const T& init = T{}) : m_data(std::vector<T>(count, init)) { validate(); }
  ~Ring() = default;

  Ring(const Ring&) = default;
  Ring(Ring&&) = default;
  Ring& operator=(const Ring&) = default;
  Ring& operator=(Ring&&) = default;

  T& next() {
    T& elem = m_data[m_index];
    m_index = (m_index + 1) % m_data.size();
    return elem;
  }

private:
  void validate() {
    if (m_data.empty()) {
      std::println(stderr, "[GRF] ERROR: Ring must contain at least one element");
      std::abort();
    }
  }
};

}

inline constexpr grf::ImageUsage operator|(grf::ImageUsage lhs, grf::ImageUsage rhs) {
  return static_cast<grf::ImageUsage>(static_cast<grf::u32>(lhs) | static_cast<grf::u32>(rhs));
}

inline constexpr grf::ImageUsage operator&(grf::ImageUsage lhs, grf::ImageUsage rhs) {
  return static_cast<grf::ImageUsage>(static_cast<grf::u32>(lhs) & static_cast<grf::u32>(rhs));
}

inline constexpr grf::ImageUsage& operator|=(grf::ImageUsage& lhs, grf::ImageUsage rhs) {
  return lhs = lhs | rhs;
}

inline constexpr grf::ImageUsage& operator&=(grf::ImageUsage& lhs, grf::ImageUsage rhs) {
  return lhs = lhs & rhs;
}

inline constexpr grf::BufferStage operator|(grf::BufferStage lhs, grf::BufferStage rhs) {
  return static_cast<grf::BufferStage>(static_cast<grf::u32>(lhs) | static_cast<grf::u32>(rhs));
}

inline constexpr grf::BufferStage operator&(grf::BufferStage lhs, grf::BufferStage rhs) {
  return static_cast<grf::BufferStage>(static_cast<grf::u32>(lhs) & static_cast<grf::u32>(rhs));
}

inline constexpr grf::BufferStage& operator|=(grf::BufferStage& lhs, grf::BufferStage rhs) {
  return lhs = lhs | rhs;
}

inline constexpr grf::BufferStage& operator&=(grf::BufferStage& lhs, grf::BufferStage rhs) {
  return lhs = lhs & rhs;
}