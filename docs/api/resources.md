# Buffer, Image, Sampler

`#include "grf/resources.hpp"`

All three are copyable shared handles. The underlying GPU resource is freed when the last handle drops. Don't destroy a resource the GPU is still using — hold handles for as long as any in-flight frame references them. Calling `device.waitIdle()` before application close will ensure that all resources are not in use before getting destroyed.

## Buffer

```cpp
Buffer buffer = device.createBuffer(Buffer::Info{
  .size   = bytes,
  .intent = BufferIntent::GPUOnly
});
```

Every buffer supports storage, index, indirect, transfer, and device-address usage; `BufferIntent` only chooses the memory type:

| Intent | Memory | `write` | `read` |
| --- | --- | --- | --- |
| `GPUOnly` | device-local | staged (needs `updateResources`) | — |
| `FrequentUpdate` | host-mapped | direct memcpy | — |
| `Readable` | host-mapped | direct memcpy | memcpy back |

### Methods

```cpp
u64 address() const;

template <typename T> void write(const T& value, u32 offset = 0) const;
template <range R>    void write(R&& range, u32 offset = 0) const;
template <typename T> T read(u32 offset = 0) const;
```

`address()` is the buffer device address for shader `buffer_reference` access. Out-of-bounds writes/reads are skipped with a logged warning. `read` returns whatever is in mapped memory at call time. Make sure to wait on the `Sync` of the GPU work that produced the data first.

`write` can either take a single trivially copyable object or a range of trivially copyable objects.

## Image

```cpp
Image image = device.createImage(Image::Info{
  .width        = w,
  .height       = h,
  .depth        = d,
  .format       = Format::rgba8_srgb,
  .usage        = ImageUsage::Sampled,   // | Storage | ColorAttachment | DepthAttachment
  .allocateMips = false
});
```

Creating with `Sampled`/`Storage` usage registers the image in the bindless heap. `allocateMips = true` allocates the full mip chain. To upload mips, either write the data directly to the mip level or call `generateMips` on a command buffer created from the graphics queue.

### Methods

```cpp
static Data getDataFromFile(const std::filesystem::path& path, bool srgb = true);

u32 width() const;
u32 height() const;
u32 depth() const;
u32 sampledHeapIndex() const;
u32 storageHeapIndex() const;

void write(std::span<const std::byte> bytes, Layout finalLayout, u32 mip = 0) const;
```

`getDataFromFile` loads LDR/16-bit/HDR files via stb_image into an `Image::Data{ width, height, format, bytes }` ready to pass to `createImage`/`write`. `write` always stages. After `updateResources`, the flushed copy transitions the image to `TransferDstOptimal`, copies `bytes` into `mip`, and transitions to `finalLayout`.

## Sampler

```cpp
Sampler sampler = device.createSampler(Sampler::Info{});
```

`Sampler::Info` exposes min/mag `Filter`, `MipmapMode`, per-axis `AddressMode`, `mipLodBias`, `anisotropyEnable` (max anisotropy comes from the device), and depth-compare (`compareEnable`, `compareOp`). Samplers always register in the bindless heap. Access them with `heapIndex()`.
