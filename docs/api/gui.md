# GUI

Controls ImGui rendering for a given swapchain. Must be created after the swapchain it is attached to.

## Construction

```cpp
GUI gui(const Device& device, const Swapchain& swapchain);
```

## Methods

```cpp
bool ownsInput() const;
void newFrame() const;
void render(CommandBuffer cmd) const;
```

`ownsInput` returns `true` if ImGui wants to capture keyboard input or mouse input

All ImGui methods to be rendered to the swapchain attached to GUI must be placed between `newFrame` and `render`. The `CommandBuffer` given to `render` must be from the graphics queue.
