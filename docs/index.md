# GRF Documentation

## Guide

- [Getting started](getting-started.md) — building GRF and writing a first application
- [Core concepts](concepts.md) — synchronization, the bindless heap, resource staging, and the contracts GRF expects you to uphold

## API reference

| Page | Types |
| --- | --- |
| [Device](api/device.md) | `Device` |
| [Window](api/window.md) | `Window`, `Key` |
| [Swapchain](api/swapchain.md) | `Swapchain`, `SwapchainImage` |
| [Queue](api/queue.md) | `Queue`, `CommandBuffer`, `Sync`, `ColorAttachment`, `DepthAttachment` |
| [Resources](api/resources.md) | `Buffer`, `Image`, `Sampler` |
| [Pipeline](api/pipeline.md) | `Shader`, `Pipeline`, `GraphicsPipelineSettings` |
| [GUI](api/gui.md) | `GUI`, `Profiler` |

GRF documents its contracts instead of enforcing them: the library does not add hidden locks, implicit barriers, or safety rails beyond what is described here. When a page says "you must", validation layers (enabled automatically in Debug builds) are the tool that tells you when you didn't.
