# Proton G Architecture Overview

This document describes the current high-level architecture of Proton G.

Proton G is an early Luau-first C++20/Vulkan runtime. The current architecture focuses on a small engine loop, script execution, fullscreen image rendering, media playback foundations, and runtime diagnostics.

---

## Runtime Layout

At build time, the source `sandbox/` folder is copied into the build output folder.

```text
ProtonG/
  src/
  sandbox/
    scripts/
    shaders/
    assets/
```

After building:

```text
cmake-build-debug/
  ProtonG.exe
  sandbox/
    scripts/
    shaders/
    assets/
```

The runtime loads scripts and assets from the copied build-output sandbox/ folder.

## Core Systems

### ProtonApplication

ProtonApplication is the main runtime orchestrator.

It owns the application lifecycle:

- initializes GLFW
- creates the window
- initializes Vulkan
- attaches runtime systems
- loads sandbox scripts
- runs the main loop
- updates scripts and media
- draws frames
- logs runtime stats
- shuts systems down in the correct order

Main loop flow:

1. calculate delta time
2. update scripts
3. update media player
4. draw Vulkan frame
5. poll GLFW events
6. check window close
7. log runtime stats

### ProtonVulkanContext

ProtonVulkanContext owns the Vulkan rendering backend.

Current responsibilities include:

- Vulkan instance creation
- window surface creation
- physical GPU selection
- logical device creation
- graphics/present queue selection
- swapchain creation
- swapchain image views
- render pass creation
- framebuffer creation
- command pool creation
- command buffer allocation
- synchronization objects
- fullscreen image pipeline
- texture upload
- image scale mode handling
- framebuffer resize handling

At v0.01, rendering is intentionally simple: Proton G displays a fullscreen textured image with configurable scaling.

Supported image scale modes:

- stretch
- fit
- fill
- native

### ProtonScriptEngine

ProtonScriptEngine embeds Luau and exposes engine functions through the global proton table.

Current responsibilities include:

- initializing the Luau VM
- loading scripts from sandbox/scripts
- detecting update(dt)
- calling Luau update every frame
- exposing selected C++ callbacks to scripts
- reporting script errors through Proton logging

Example script:

```lua
--!strict

print("Proton G sandbox script loaded.")

proton.setImageScaleMode("fit")
proton.setClearColor(0.05, 0.08, 0.16, 1.0)

function update(dt: number)
end
```

Declaration-only files such as .d.luau should be used for editor/type support and should not be executed as runtime scripts.

### ProtonMediaPlayer

ProtonMediaPlayer is the generic media playback system.

Current responsibilities include:

- decoding audio through miniaudio
- owning the miniaudio implementation
- playing audio
- tracking submitted audio frames
- selecting image-sequence frames from audio timing
- loading image frames through ProtonImageLoader
- uploading selected frames to ProtonVulkanContext

The MediaPlayer is meant to be content-agnostic. It should not contain hardcoded demo paths or project-specific asset names.

Long-term, media playback should be started through generic Luau APIs such as:

```lua
proton.media.playImageSequence(
    "sandbox/assets/demo/frames/output_%04d.jpg",
    "sandbox/assets/demo/audio/audio.wav",
    1,
    300,
    30
)
```

### ProtonImageLoader

ProtonImageLoader loads image files from disk into CPU-side image data.

Current responsibilities include:

- loading image files
- decoding pixels through stb_image
- returning width, height, channel count, and pixel data
- providing image data to Vulkan texture upload code

At v0.01, this is mainly used for fullscreen image rendering and MediaPlayer image-sequence frames.

### ProtonSystemStats

ProtonSystemStats gathers runtime diagnostics.

Current responsibilities include:

- process CPU usage
- process memory usage
- system memory usage
- formatted memory output for logs

ProtonApplication logs these stats once per second alongside FPS and frame time.

### ProtonLog

ProtonLog is the shared logging utility.

It is used by the runtime systems to report:

- startup progress
- Vulkan initialization
- script loading
- media playback
- asset loading
- runtime stats
- shutdown progress
- errors

---

## High-Level Data Flow

### Startup

1. **main.cpp**
   - creates Proton::Application
   - calls Application::Run()

2. **Application::Startup()**
   - initializes logging
   - initializes GLFW
   - creates window
   - initializes Vulkan
   - attaches MediaPlayer to Vulkan
   - registers Luau callbacks
   - loads scripts from sandbox/scripts
   - enters running state

### Per-Frame Update

**Application::Run()**
- calculate deltaTime
- **Application::Tick(deltaTime)**
  - ScriptEngine.Update(deltaTime)
  - MediaPlayer.Update(deltaTime)
  - stats logging
- VulkanContext.DrawFrame()
- glfwPollEvents()

### Script Flow

**sandbox/scripts/*.luau**
- loaded by ProtonScriptEngine
- can call proton APIs
- optional update(dt) called every frame

Example:

```lua
proton.setClearColor(0.05, 0.08, 0.16, 1.0)
proton.setImageScaleMode("fit")
```

### Image Rendering Flow

1. Luau or MediaPlayer requests image
2. ProtonImageLoader loads file
3. ProtonVulkanContext uploads texture
4. Vulkan draws fullscreen textured quad

### Media Playback Flow

1. MediaPlayer receives image sequence config
2. starts audio decoder/device
3. tracks audio-submitted frames
4. calculates current frame from audio time
5. loads selected image frame
6. uploads it to Vulkan
7. Vulkan displays it

---

## Current v0.01 Boundaries

Proton G v0.01 is not a full 2D engine yet.

Current focus:

- bootable Vulkan runtime
- Luau script loading
- fullscreen image rendering
- image scaling
- media playback foundation
- runtime diagnostics

Not yet included:

- sprite system
- input system
- scene graph
- entity/component system
- asset manager
- editor
- physics
- 3D rendering

---

## Planned Direction

### v0.02

The next milestone is a basic 2D runtime:

- keyboard input
- mouse input
- sprite rendering
- position, scale, and rotation
- multiple image assets
- Luau-controlled 2D objects

### Later

Future milestones may add:

- scene management
- asset handles
- hot reload
- editor tooling
- project files
- 3D rendering
- deeper Luau IDE/editor integration
