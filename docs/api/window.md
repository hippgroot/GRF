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

bool isPressed(MouseButton button) const;
bool isJustPressed(MouseButton button) const;
bool isJustReleased(MouseButton button) const;

vec2 cursorPosition() const;
vec2 cursorDelta() const;

void onResize(std::function<void(u32, u32)>);
```

Call `pollEvents()` once per frame. It refreshes the *pressed / just-pressed / just-released* keyboard state that the `is*` queries read.

`cursorPosition` and `cursorDelta` are both in normalized device coordinates (x and y are between -1 and 1).

`width()` and `height()` track the framebuffer size of the window.

`onResize` is called when the framebuffer is resized. It is there for things like adjusting camera aspect ratio or recreating depth resources.
