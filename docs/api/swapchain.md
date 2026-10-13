# Swapchain

`#include "grf/swapchain.hpp"`

Presentable images for a `Window`. Non-copyable and non-movable. Create after `Device` and `Window` and make sure it is destroyed before them.

```cpp
Swapchain swapchain(device, window);
Swapchain swapchain(device, window, Swapchain::Info{
  .format     = Format::bgra8_srgb,
  .colorspace = ColorSpace::srgb_nonlinear,
  .vsync      = true
});
```

The requested format/colorspace fall back to a supported combination when unavailable. HDR colorspaces (`hdr10_st2084`, `extended_srgb_linear`, …) can be requested where the platform supports them.

## Methods

```cpp
Format format() const;
ColorSpace colorspace() const;
bool vsync() const;
bool vsyncOnly() const;
void enableVsync();
void disableVsync();

std::pair<SwapchainImage, Sync> acquireNextImage(u64 timeout_ns = u64_max) const;
```

`acquireNextImage` returns the image to render into this frame plus a binary-semaphore `Sync` that must be passed as a wait to the `queue.submit` that renders to the image.

`enableVsync`/`disableVsync` may be called from any thread; they flag a recreation that happens on the next acquire. `disableVsync` prefers mailbox, falls back to immediate, and stays on FIFO (with a warning) when `vsyncOnly()` is true.

When the swapchain becomes suboptimal or out of date (e.g. window resize, borderless toggle), recreation is only ever performed lazily inside `acquireNextImage`, which retries the acquire until it succeeds — the resize callback and out-of-date presents merely flag it. The swapchain is therefore only touched by the thread that acquires and presents, so `acquireNextImage`/`present` may live on a dedicated render thread while the window is polled on the main thread.

## SwapchainImage

```cpp
Format format() const;
u32 width() const;
u32 height() const;
```

`width()`/`height()` are the true pixel dimensions — use these for `beginRendering` and for sizing attachments like depth buffers (they can exceed `Window::width()/height()` on high-DPI displays).

A `SwapchainImage` is transient: acquire one per frame, transition it to `ColorAttachmentOptimal`, render, transition to `PresentSrc`, then hand it to `queue.present`.
