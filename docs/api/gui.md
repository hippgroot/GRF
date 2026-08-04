# GUI

`#include "grf/gui.hpp"`

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

## Profiler

Measures CPU frame times and named GPU scopes (via timestamp queries) and draws the results in an ImGui window. Non-copyable and non-movable.

```cpp
Profiler profiler(device);
Profiler profiler(device, Profiler::Info{
  .maxSamples    = 240,
  .maxGpuSamples = 16,
  .flightFrames  = 2
});
```

### `Profiler::Info`

| Field | Default | Meaning |
| --- | --- | --- |
| `maxSamples` | `240` | Frame time samples kept for the plot |
| `maxGpuSamples` | `16` | Maximum GPU scopes per frame |
| `flightFrames` | `2` | Must match your render loop's frames-in-flight count |

### Methods

```cpp
void logFrameTime(f32 time) const;
void beginFrame(CommandBuffer cmd) const;
void beginScope(CommandBuffer cmd, const str& name) const;
void endScope(CommandBuffer cmd) const;

void draw() const;
```

`logFrameTime` takes the frame's delta time in seconds — `device.beginFrame()` returns exactly this. Call it once per frame.

`beginFrame` must be recorded once per frame, before any scopes, and only after the frame's flight `Sync` has been waited on (see the [frames-in-flight pattern](../concepts.md#frames-in-flight-pattern)). It reads back the timestamps recorded `flightFrames` frames earlier; that wait is what guarantees they are available.

`beginScope`/`endScope` record a named GPU timing scope. Scopes may nest -- each `endScope` closes the innermost open scope and must all be closed by the time the next `beginFrame` is recorded. A frame may record at most `maxGpuSamples` scopes. Times are measured top-of-pipe to bottom-of-pipe, so scopes whose GPU work overlaps are not additive.

`draw` submits the profiler window and must be placed between `gui.newFrame()` and `gui.render()`. The frame time, fps, and GPU scope times it shows are averages that refresh once per second; the frame time plot updates every frame.
