# Window

`#include "grf/window.hpp"`

A GLFW-backed OS window with keyboard state tracking. Non-copyable, non-movable.

```cpp
Window window(Window::Info{
  .width  = 1280,
  .height = 720,
  .title  = "GRF Application"
});
```

## Methods

```cpp
bool shouldClose() const;
u32 width() const;
u32 height() const;

void pollEvents();
bool isPressed(Key key) const;
bool isJustPressed(Key key) const;
bool isJustReleased(Key key) const;
```

Call `pollEvents()` once per frame. It refreshes the *pressed / just-pressed / just-released* keyboard state that the `is*` queries read.

`width()`/`height()` are the window's logical size. On high-DPI displays the swapchain's pixel dimensions can be larger. It is better to take render sizes from the acquired `SwapchainImage` rather than the window.
