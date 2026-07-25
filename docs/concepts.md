# Core Concepts

GRF's design philosophy is **explicit, user-called contracts over hidden safety machinery**. The library never inserts a wait, a barrier, or a flush you didn't ask for (with one documented exception in staging). This page is the list of those contracts.

## Device and queues

`Device` owns the Vulkan instance, physical device selection, the allocator, the bindless descriptor heap, and the staging system. It exposes two queues:

- `device.graphicsQueue()` — graphics + compute + present capable.
- `device.computeQueue()` — a separate compute queue when the hardware has one (async compute), otherwise a graphics-capable family (may be the same as the graphics queue).

A third transfer queue exists internally and is used exclusively by the staging system. This will be an async transfer queue if the hardware has one, otherwise it will target the next best option (an async compute queue or just a graphics queue).

## Synchronization: `Sync`

A `Sync` is an object that manages a point on a timeline semaphore or carries a binary semaphore. What it contains is managed internally by GRF. It simplifies the normal synchronization pattern by having renderers rely on just a few `Sync`s rather than a lot of fences and binary semaphores.

`queue.submit()`, `swapchain.acquireNextImage()`, and `device.updateResources()` all produce `Sync`s that should be waited on during a `queue.submit()` or `queue.present()` call. `device.wait({ syncs })` blocks the thread until all the syncs are signaled.

You are required to manage the synchronization state of your application. The frames-in-flight pattern below is the simplest synchronization shape to follow.

### Frames in flight pattern

```cpp
// OUTSIDE MAIN LOOP
Ring<CommandBuffer> cmdRing = queue.getCmdRing(flightFrames);
Ring<Sync> syncRing = queue.getSyncRing(flightFrames);

// INSIDE MAIN LOOP
auto& sync = syncRing.next();
device.wait({ sync });
auto& cmd = cmdRing.next();
...
sync = queue.submit({ cmd }, { /* any additional syncs to wait on */ });
```

`Sync`s in `syncRing` ensure that the beginning of each frame contains a fresh reusable state after its `Sync` is waited on. Ensure that the `Sync` obtained from `syncRing` is stored in a reference (`auto&`) rather than copied; otherwise the value will not be updated in `syncRing` and the frame synchronization will break.

## The bindless heap

There are no descriptor sets in the API. `Device` maintains one global heap with three unsized bindings, automatically bound at `cmd.begin()`:

| Binding | Contents | Index accessor |
| --- | --- | --- |
| 0 | sampled images (`texture2D[]`) | `image.sampledHeapIndex()` |
| 1 | storage images (`image2D[]`) | `image.storageHeapIndex()` |
| 2 | samplers (`sampler[]`) | `sampler.heapIndex()` |

Images are registered when created with `ImageUsage::Sampled` and/or `ImageUsage::Storage`; samplers always. Pass the indices to shaders in push constants and index the arrays (declare `GL_EXT_nonuniform_qualifier` when the index is divergent).

Buffers don't need the heap at all: `buffer.address()` returns the buffer device address, which shaders consume as a `buffer_reference`.

Buffer addresses and heap indices should be passed in via push constants. Push constants have a **128-byte budget** shared by all stages.

## Resource staging

Where a write actually goes depends on the buffer's `BufferIntent`:

- `FrequentUpdate` and `Readable` buffers are host-mapped: `write()` is an immediate memcpy, `read()` (`Readable` buffers only) a memcpy back. No staging, no `updateResources()` involvement. It is on you to not overwrite data the GPU is currently reading — use frames-in-flight buffer rings (`device.createBufferRing`).
- `GPUOnly` buffers and all `Image` writes are staged. The data is copied into an internal staging ring immediately, and the GPU-side copy is recorded and submitted later when you call `device.updateResources()`.

`device.updateResources()` submits all pending copies on the internal transfer queue and returns a `Sync`. Use that `Sync` as a wait to the submit that consumes the data:

```cpp
Sync resourceSync = device.updateResources();
...
queue.submit({ cmd }, { resourceSync });
```

Calling it with nothing staged is free and returns the last flush's sync. The intended pattern is to always call it once per frame.

If a single frame stages more data than one staging buffer holds, GRF flushes early to make room (and blocks the calling thread if it must recycle a staging buffer the GPU hasn't finished with). Size the ring via `Device::Info::stagingSize_MB` (default 64) and `stagingCount` (default 2) so this stays rare. A single `write()` larger than one staging buffer is an error and will warn you to increase the staging buffer size.

`Image::write(bytes, finalLayout, mip)` also transitions the image for convenience. The flushed copy transitions it to `TransferDstOptimal`, copies, then transitions to `finalLayout`.

## Image layouts

Each `Image` and `SwapchainImage` tracks its current layout. `cmd.transition(img, layout)` records a pipeline barrier from the tracked layout to the requested one, choosing stage/access scopes from both layouts, and updates the tracked layout at record time.

`cmd.generateMips(img, finalLayout)` blits the mip chain from mip 0 and leaves the whole image in `finalLayout`. It will need to be called whenever you would like to regenerate mips for a procedurally generated texture, or on the first frame if you would like to generate mips for an image created with `allocateMips = true`.
