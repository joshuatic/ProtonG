# Proton G Architecture Overview

This document describes the current high-level architecture of Proton G.

Proton G is an early Luau-first C++20/Vulkan runtime. The current architecture focuses on a small engine loop, Luau scripting, Vulkan rendering, immediate-mode 2D drawing, sprite rendering, runtime object handles, input, collision helpers, media playback foundations, and runtime diagnostics.

## Runtime Layout

At build time, the source `sandbox/` folder is copied into the build output folder.

**Runtime source layout:**
```
ProtonG/
└── src/
    └── sandbox/
        ├── scripts/
        ├── shaders/
        └── assets/
```

**After building:**
```
cmake-build-debug/
├── ProtonG.exe
└── sandbox/
    ├── scripts/
    ├── shaders/
    └── assets/
```

The runtime loads scripts, shaders, and assets from the copied build-output `sandbox/` folder.

## Core Systems

### `ProtonApplication`

`ProtonApplication` is the main runtime orchestrator.

It owns the application lifecycle:
- Initializes logging.
- Initializes GLFW.
- Creates the window.
- Initializes Vulkan.
- Owns and connects runtime systems.
- Registers Luau callbacks.
- Loads sandbox scripts.
- Runs the main loop.
- Updates scripts and media.
- Submits frames to Vulkan.
- Logs runtime stats.
- Shuts systems down in the correct order.

**Main loop flow:**
1. Calculate delta time.
2. Update Luau scripts.
3. Update media playback.
4. Draw the Vulkan frame.
5. Poll GLFW events.
6. Check window close state.
7. Log runtime stats.

`ProtonApplication` currently coordinates:
- `ProtonVulkanContext`
- `ProtonScriptEngine`
- `ProtonMediaPlayer`
- `ProtonInput`
- `ProtonDraw2D`
- `ProtonSpriteManager`
- `ProtonObjectManager`
- `ProtonSystemStats`

### `ProtonVulkanContext`

`ProtonVulkanContext` owns the Vulkan rendering backend.

Current responsibilities include:
- Vulkan instance creation.
- Window surface creation.
- Physical GPU selection.
- Logical device creation.
- Graphics/present queue selection.
- Swapchain creation.
- Swapchain image views.
- Render pass creation.
- Framebuffer creation.
- Command pool creation.
- Command buffer allocation.
- Synchronization objects.
- Fullscreen image pipeline.
- Rect/shape 2D pipeline.
- Sprite2D textured quad pipeline.
- Sprite descriptor set layout.
- Descriptor pool management.
- Texture upload.
- Sprite GPU texture cache by handle.
- Image scale mode handling.
- Framebuffer resize handling.
- Draw2D vertex buffer updates.
- Sprite2D vertex buffer updates.
- Consecutive same-texture sprite batching.
- Unified cross-renderer render queue.

The renderer supports multiple rendering paths:
- Fullscreen image rendering.
- Immediate-mode 2D shape rendering.
- Sprite/textured quad rendering.

### Unified Render Queue

The unified render queue combines shape batches and sprite batches into one shared render order.

Draw commands still enter through their own systems:
- Shapes go through `ProtonDraw2D`.
- Sprites go through `ProtonSpriteManager`.

Before rendering, Vulkan builds a unified queue containing:
- `Draw2D` batch entries.
- `Sprite2D` batch entries.

Entries are sorted by layer. Lower layers render first. Higher layers render later.

When shape and sprite batches share the same layer, shape batches currently render before sprite batches.

This replaced the older shape-first/sprite-second style ordering model.

### `ProtonDraw2D`

`ProtonDraw2D` collects immediate-mode shape commands from Luau.

**Current supported draw calls:**
- `proton.draw.rect`
- `proton.draw.circle`
- `proton.draw.polygon`

**Current responsibilities include:**
- Collecting per-frame shape commands.
- Storing rectangle/polygon/circle draw requests.
- Supporting stable draw IDs.
- Supporting color and alpha.
- Supporting layer values.
- Converting circles into high-point polygons.
- Clearing transient draw commands per frame.
- Providing draw command data to the Vulkan backend.

The draw API is immediate-mode. Scripts must submit shapes every frame inside `update(dt)`.

### `ProtonSpriteManager`

`ProtonSpriteManager` manages sprite loading and sprite draw commands.

**Current supported APIs:**
- `proton.sprite.load`
- `proton.sprite.draw`
- `proton.sprite.drawOn`
- `proton.sprite.drawObject`
- `proton.sprite.drawOnObject`

**Current responsibilities include:**
- Loading sprite image data.
- Returning sprite handles to Luau.
- Storing CPU-side image data.
- Tracking sprite metadata.
- Collecting per-frame sprite draw commands.
- Supporting sprite position.
- Supporting sprite size/scale.
- Supporting sprite rotation.
- Supporting tint/color.
- Supporting alpha.
- Supporting layer values.
- Supporting multiple sprite assets at once.
- Reusing sprite handles across multiple draw calls.
- Providing sprite command data to the Vulkan backend.

The Vulkan backend uploads sprite textures to the GPU and caches them by handle.

Sprite rendering supports consecutive same-texture batching. Multiple consecutive sprite draw commands using the same texture handle and layer can be collapsed into fewer GPU batches without changing script behavior.

### `ProtonInput`

`ProtonInput` tracks keyboard and mouse state.

**Current supported APIs:**
- `proton.input.isKeyDown`
- `proton.input.isKeyPressed`
- `proton.input.isMouseButtonDown`
- `proton.input.isMouseButtonPressed`
- `proton.input.getMouseX`
- `proton.input.getMouseY`

**Current responsibilities include:**
- Tracking currently held keys.
- Tracking keys pressed this frame.
- Tracking currently held mouse buttons.
- Tracking mouse buttons pressed this frame.
- Tracking mouse position.
- Clearing one-frame input state at the correct time.
- Providing input state to Luau through `ProtonScriptEngine`.

Input is used by Luau gameplay tests for movement, jumping, clicking, and simple interaction.

### `ProtonObjectManager`

`ProtonObjectManager` stores simple runtime object handles.

**Current supported APIs:**
- `proton.object.create`
- `proton.object.setPosition`
- `proton.object.setSize`
- `proton.object.getX`
- `proton.object.getY`
- `proton.object.getWidth`
- `proton.object.getHeight`

**Runtime objects currently store:**
- Object handle.
- Name.
- X position.
- Y position.
- Width.
- Height.

These are lightweight rectangle transform handles, not a full scene graph or Roblox-style instance system yet.

They are used to simplify Luau gameplay logic and support object-aware sprite helpers.

### `ProtonScriptEngine`

`ProtonScriptEngine` embeds Luau and exposes engine functions through the global `proton` table.

**Current responsibilities include:**
- Initializing the Luau VM.
- Loading scripts from `sandbox/scripts`.
- Skipping declaration-only files where appropriate.
- Detecting `update(dt)`.
- Calling Luau `update(dt)` every frame.
- Exposing selected C++ callbacks to scripts.
- Reporting script errors through Proton logging.
- Exposing window APIs.
- Exposing input APIs.
- Exposing draw APIs.
- Exposing sprite APIs.
- Exposing object APIs.
- Exposing collision APIs.
- Exposing media APIs.
- Exposing renderer configuration APIs.

Declaration-only files such as `.d.luau` are for editor/type support and should not be treated as runtime gameplay scripts.

### `ProtonCollision` Helpers

The collision API currently exposes simple helper functions to Luau.

**Current supported API:**
- `proton.collision.rectsOverlap`

**Current responsibilities include:**
- Providing basic rectangle overlap checks.
- Supporting simple gameplay tests.
- Enabling player-vs-hazard and player-vs-enemy collision behavior from Luau.

This is not a physics engine. It is a small helper layer for basic 2D runtime tests.

### `ProtonMediaPlayer`

`ProtonMediaPlayer` is the generic media playback system.

**Current responsibilities include:**
- Decoding audio through miniaudio.
- Owning the miniaudio implementation.
- Playing audio.
- Tracking submitted audio frames.
- Selecting image-sequence frames from audio timing.
- Loading image frames through `ProtonImageLoader`.
- Uploads selected frames to `ProtonVulkanContext`.

The `MediaPlayer` is meant to be content-agnostic. It should not contain hardcoded demo paths or project-specific asset names.

Long-term, media playback should be started through generic Luau APIs such as:
```lua
proton.media.playImageSequence("sandbox/assets/demo/frames/output_%04d.jpg", "sandbox/assets/demo/audio/audio.wav", 1, 300, 30)
```

### `ProtonImageLoader`

`ProtonImageLoader` loads image files from disk into CPU-side image data.

**Current responsibilities include:**
- Loading image files.
- Decoding pixels through `stb_image`.
- Returning width, height, channel count, and pixel data.
- Providing image data to Vulkan texture upload code.
- Supporting sprite image loading.
- Supporting fullscreen/media image loading.

This system is used by both media playback and sprite loading.

### `ProtonSystemStats`

`ProtonSystemStats` gathers runtime diagnostics.

**Current responsibilities include:**
- Process CPU usage.
- Process memory usage.
- System memory usage.
- Formatted memory output for logs.

`ProtonApplication` logs these stats once per second alongside FPS and frame time.

### `ProtonLog`

`ProtonLog` is the shared logging utility.

It is used by runtime systems to report:
- Startup progress.
- Vulkan initialization.
- Script loading.
- Input events.
- Asset loading.
- Sprite loading.
- Texture upload.
- Draw command requests.
- Runtime stats.
- Shutdown progress.
- Errors.

Temporary per-frame debug spam should not remain enabled after validation.

## High-Level Data Flow

### Startup

1. **main.cpp**
    - Creates `Proton::Application`.
    - Calls `Application::Run()`.
2. **Application::Startup()**
    - Initializes logging.
    - Initializes GLFW.
    - Creates the window.
    - Initializes Vulkan.
    - Attaches runtime systems.
    - Registers Luau callbacks.
    - Loads scripts from `sandbox/scripts`.
    - Enters the running state.

### Per-Frame Update

**Application::Run():**
1. Calculates delta time.
2. Calls `Application::Tick(deltaTime)`.
3. Draws a Vulkan frame.
4. Polls GLFW events.

**Application::Tick(deltaTime):**
1. Updates Luau scripts.
2. Updates media playback.
3. Logs runtime stats when needed.

**VulkanContext::DrawFrame():**
1. Acquires swapchain image.
2. Updates fullscreen/media texture if needed.
3. Updates `Draw2D` vertex data.
4. Updates `Sprite2D` vertex data.
5. Uploads missing sprite textures.
6. Builds sprite batches.
7. Builds unified render queue.
8. Records command buffer.
9. Submits commands.
10. Presents frame.

### Script Flow

Scripts in `sandbox/scripts/*.luau` are loaded by `ProtonScriptEngine`.

**Scripts can:**
- Configure the window.
- Set clear color.
- Read input.
- Load sprites.
- Create runtime objects.
- Update object positions.
- Test collisions.
- Draw shapes.
- Draw sprites.
- Draw sprites relative to objects.
- Run gameplay logic inside `update(dt)`.

**Example script flow:**
1. Load sprite assets.
2. Create player object.
3. In `update(dt)`, read input.
4. Move player.
5. Apply gravity.
6. Check collision.
7. Submit draw commands.

### Shape Rendering Flow
1. Luau calls `proton.draw.rect`, `proton.draw.circle`, or `proton.draw.polygon`.
2. `ProtonScriptEngine` forwards the call to `ProtonDraw2D`.
3. `ProtonDraw2D` stores the draw command for the current frame.
4. `ProtonVulkanContext` converts shape commands into vertices.
5. Shape vertices are grouped into `Draw2D` layer batches.
6. `Draw2D` batches are added to the unified render queue.
7. Vulkan draws the batches in queue order.

### Sprite Rendering Flow
1. Luau calls `proton.sprite.load`.
2. `ProtonSpriteManager` loads image data through `ProtonImageLoader`.
3. A sprite handle is returned to Luau.
4. Luau calls `proton.sprite.draw` or an object-aware sprite helper.
5. `ProtonSpriteManager` stores sprite draw commands for the current frame.
6. `ProtonVulkanContext` uploads missing sprite textures to the GPU.
7. Sprite draw commands are converted into textured quad vertices.
8. Consecutive same-texture sprite commands are batched.
9. Sprite batches are added to the unified render queue.
10. Vulkan draws the batches in queue order.

### Object Flow
1. Luau calls `proton.object.create`.
2. `ProtonObjectManager` creates a lightweight object handle.
3. Luau reads or updates the object transform.
4. Sprite helpers can draw using the object transform.
5. Collision helpers can test object rectangle data.

Objects are currently simple transform containers. They do not own rendering, scripts, physics, or components yet.

### Collision Flow
1. Luau gathers rectangle data.
2. Luau calls `proton.collision.rectsOverlap`.
3. C++ returns whether the rectangles overlap.
4. Luau decides gameplay response.

**Example responses:**
- Tint player sprite.
- Draw red screen flash.
- Reset position.
- Trigger print/log message.
- Block movement.
- Collect pickup.

### Media Playback Flow
1. `MediaPlayer` receives image sequence config.
2. Starts audio decoder/device.
3. Tracks audio-submitted frames.
4. Calculates current frame from audio time.
5. Loads selected image frame.
6. Uploads it to Vulkan.
7. Vulkan displays it.

Media playback is currently separate from the sprite/object runtime.

## Current v0.02 Boundaries

Proton G v0.02 is a basic 2D runtime milestone.

**Current focus:**
- Bootable Vulkan runtime.
- Luau script loading.
- Immediate-mode 2D shape rendering.
- Sprite loading and rendering.
- Multiple sprite assets.
- Sprite tint, alpha, rotation, size, and layers.
- Keyboard and mouse input.
- Window APIs.
- Simple runtime object handles.
- Basic rectangle collision helper.
- Sprite batching.
- Unified cross-renderer render queue.
- Runtime diagnostics.
- Media playback foundation.

**Not yet included:**
- Full scene graph.
- Entity/component system.
- Physics engine.
- Asset registry.
- Hot reload.
- Editor tooling.
- Project files.
- UI system.
- Text rendering API.
- 3D rendering.
- Full Luau IDE/editor integration.

## Planned Direction

### v0.03

The next likely milestone is API polish and runtime structure.

**Possible v0.03 goals:**
- Cleaner runtime object ownership.
- Object rotation/transform helpers.
- More collision helpers.
- Better asset handle workflow.
- Cleaner sandbox examples.
- Better debug toggles.
- Renderer logging controls.
- Basic scene/demo organization.
- Runtime stability cleanup after v0.02.

### Later

Future milestones may add:
- Scene management.
- Asset registry.
- Hot reload.
- Project files.
- Editor tooling.
- UI tooling.
- Text rendering.
- 3D rendering.
- More advanced rendering features.
- Luau IDE/editor integration.
- Better packaging and runtime distribution.