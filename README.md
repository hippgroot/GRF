# GRF — Groot Rendering Framework

A modern Vulkan 1.4 rendering framework for C++26 targeting dynamic rendering, bindless descriptor indexing, buffer device addresses, timeline semaphores, and indirect buffers to build advanced and performant renderers, all through a lightweight RAII interface.

## Features

- **Resource Management** - A single unified descriptor heap for images combined with buffer device addresses completely removes manual management of descriptor sets. Pass your shaders buffer addresses or image heap indexes to use your resources instead.
- **Internal Resource Uploads** - Resource uploads are handled by GRF internally through a resource manager, so no dealing with transfer queue setup. Just call `Buffer::write()` or `Image::write()` and GRF will stage the upload for you and give you a `Sync` object to wait on during your next queue submission.
- **Simplified Synchronization** - Internally, GRF uses timeline semaphores and binary semaphores. No fences. Combining both semaphores into a single `Sync` object simplifies the synchronization pattern to just a couple `Sync` objects each frame rather than dozens of semaphore and fence objects scattered throughout the code.
- **Dynamic Rendering** - Specify the extent, color, and depth attachments with `CommandBuffer::beginRendering()` once, rather than dealing with renderpasses and framebuffers.
- **RAII Based** - Don't worry about missed frees or Vulkan object destruction since GRF uses Vulkan RAII and the pimpl pattern with smart pointers to manage resource lifetimes.
- **Windowing** - GRF uses GLFW to create windows, handle input, and manage swapchain reconstruction internally, removing the need for a large collection of window callbacks.

## Requirements

- Vulkan (either through the SDK or a package manager installation) with shaderc
- A C++26 compiler
- CMake >=4.2

## Building

```sh
git clone --recursive https://github.com/hippgroot/GRF.git
cd GRF
cmake -B build
cmake --build build
```

Examples are built by default; disable them with `-DGRF_BUILD_EXAMPLES=OFF`.

Validation layers are enabled automatically on debug builds. Set the CMake build type with `-DCMAKE_BUILD_TYPE=<Release/Debug>` accordingly.

### Installing

```sh
cmake --build build --target install
```

Consume the installed package with `find_package(grf CONFIG)` and link `GRF::grf`.

Use `cmake --build build --target uninstall` to uninstall.

## Examples

| Example | Shows |
| --- | --- |
| [`triangle`](examples/triangle) | Hello World of GPU programming. Showcases the GRF syntax |

## Documentation

- [Getting started](docs/getting-started.md)
- [Core concepts](docs/concepts.md) — synchronization, the bindless heap, the staging contract
- [API reference](docs/api)

## License

GRF is licensed under the [MIT License](LICENSE). Bundled third-party dependencies keep their own licenses: [GLFW](https://github.com/glfw/glfw) (zlib), [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) (MIT), and [stb](https://github.com/nothings/stb) (MIT / public domain).
