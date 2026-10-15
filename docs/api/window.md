# Window

`#include "grf/window.hpp"`

A GLFW-backed OS window with keyboard state tracking. Non-copyable, non-movable.

```cpp
Window window(Window::Info{
  .width  = 1280,
  .height = 720,
  .title  = "GRF Application",
  .mode   = Window::Mode::Windowed
});
```

## Modes

```cpp
enum class Window::Mode : u8 {
  Windowed,
  Borderless
};
```

`Borderless` covers a whole monitor using the monitor's current video mode, so the display never changes resolution or refresh rate (GLFW's "windowed full screen"). Unlike a plain undecorated window, it sits above the macOS menu bar/Dock and the Windows taskbar. Windows do not auto-iconify when they lose focus, so alt-tabbing out of borderless leaves the window in place.

Creating a window in `Borderless` mode uses the primary monitor. `setMode(Mode::Borderless)` uses the monitor that overlaps the window the most. Switching back to `Windowed` restores the position and size the window had before it went borderless (or `width × height` centered on the monitor if it started borderless).

`setMode` must be called on the main thread. The resulting framebuffer resize flags any registered swapchains for lazy recreation, just like a manual resize.

## Methods

```cpp
bool shouldClose() const;
u32 width() const;
u32 height() const;

Mode mode() const;
void setMode(Mode mode);

void pollEvents();

bool isPressed(Key key) const;
bool isJustPressed(Key key) const;
bool isJustReleased(Key key) const;

bool isPressed(MouseButton button) const;
bool isJustPressed(MouseButton button) const;
bool isJustReleased(MouseButton button) const;

vec2 cursorPosition() const;
vec2 cursorDelta() const;
vec2 cursorMotion() const;

bool cursorCaptured() const;
void setCursorCaptured(bool captured);

void onResize(std::function<void(u32, u32)>);
```

Call `pollEvents()` once per frame. It refreshes the *pressed / just-pressed / just-released* keyboard state that the `is*` queries read.

`cursorPosition` and `cursorDelta` are both in normalized device coordinates (x and y are between -1 and 1). `cursorMotion` is the movement since the last `pollEvents()` in screen coordinates (pixels on a non-scaled display) and doesn't depend on the window size, which is what mouse-look wants.

`width()` and `height()` track the framebuffer size of the window.

`onResize` is called when the framebuffer is resized. It is there for things like adjusting camera aspect ratio or recreating depth resources.

## Cursor capture

`setCursorCaptured(true)` hides the cursor and locks it to the window (`GLFW_CURSOR_DISABLED`), so `cursorMotion` keeps reporting movement past the window edges. Raw (unaccelerated) mouse motion is enabled while captured if the platform supports it. `setCursorCaptured(false)` restores the normal cursor. Motion from the `pollEvents()` right after a change is discarded, so the jump from recentering the cursor never shows up as movement. Calling it with the current state does nothing, so it is safe to call every frame. Main thread only.

Dear ImGui still receives mouse input while the cursor is captured. Set `ImGuiConfigFlags_NoMouse` for as long as the GUI should ignore it.
