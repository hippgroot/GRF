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
DrawData snapshot() const;
void render(CommandBuffer cmd) const;
void render(CommandBuffer cmd, const DrawData& drawData) const;
```

`ownsInput` returns `true` if ImGui wants to capture keyboard input or mouse input

All ImGui methods to be rendered to the swapchain attached to GUI must be placed between `newFrame` and either `render(cmd)` or `snapshot()`. The `CommandBuffer` given to `render` must be from the graphics queue.

## Threaded rendering

`render(cmd)` finalizes and draws the current frame's UI in one call — the single-threaded path.

For a pipelined main/render thread split, use `snapshot()` + `render(cmd, drawData)` instead:

- `snapshot()` finalizes the current ImGui frame (`ImGui::Render()`) and returns a `DrawData`: a movable, non-copyable deep copy of the frame's draw lists. It is safe to hand to another thread while the main thread starts the next `newFrame()`.
- `render(cmd, drawData)` records the snapshot into the command buffer and may run on a render thread.

Threading contract: `ownsInput()`, `newFrame()` and `snapshot()` must be called from the thread that owns the window (main thread). Only `render(cmd, drawData)` may run elsewhere, and only on one thread at a time.

Texture create/update requests (in practice just the font atlas, on the first frame) are processed inside `snapshot()` on the main thread, serialized against the render thread's submits through the queue's internal mutex — the snapshot itself carries no shared texture state.

`Profiler` is internally synchronized for this split: `logFrameTime`/`beginFrame`/`beginScope`/`endScope` may run on the render thread while `draw()` runs on the main thread.

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
