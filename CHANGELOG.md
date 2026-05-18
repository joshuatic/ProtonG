# Changelog

All notable changes to Proton G will be documented in this file.

This project follows milestone-style versioning while the engine is early in development.

## Format:

```text
vMAJOR.MINOR
```

### Example:

- v0.01
- v0.02
- v0.03

---

## [Unreleased]

### Added
- Planned generic Luau media APIs.
- Planned cleanup for public repository release.
- Planned basic 2D runtime works for v0.02.

### Changed
- Removed Bad Apple as a hardcoded engine demo target.
- Moved toward generic sandbox-driven runtime behavior.

### Removed
- Hardcoded demo-specific media startup from the engine runtime.

## [v0.01] – Asset Runtime

### Added
- Initial Proton G application runtime.
- GLFW window creation.
- Vulkan instance creation.
- Vulkan window surface creation.
- Vulkan physical device selection.
- Vulkan logical device creation.
- Vulkan swapchain creation.
- Vulkan swapchain image views.
- Vulkan render pass.
- Vulkan framebuffers.
- Vulkan command pool.
- Vulkan command buffers.
- Vulkan synchronization objects.
- Fullscreen textured image rendering pipeline.
- Texture descriptor set layout.
- Texture descriptor pool.
- Runtime texture upload support.
- Image loading through the Proton image loader.
- Image scale modes:
  - stretch
  - fit
  - fill
  - native
- Luau script engine initialization.
- Sandbox script loading from `sandbox/scripts`.
- Luau `update(dt)` detection and runtime updates.
- Luau-exposed clear color API.
- Luau-exposed image scale mode API.
- MediaPlayer foundation.
- Audio decoding and playback foundation through miniaudio.
- Sandbox folder copy step during build.
- Runtime FPS logging.
- Runtime CPU and memory stats logging.
- Window title FPS display.
- Improved `.gitignore` for public repository cleanup.

### Changed
- Consolidated runtime content under the `sandbox/` folder.
- Moved scripts and shaders into the sandbox runtime layout.
- Reworked early media playback experiments into a generic MediaPlayer system.
- Removed the old hardcoded audio engine path from the main build.
- Made `ProtonMediaPlayer.cpp` the owner of miniaudio implementation.
- Cleaned up runtime startup ordering.
- Cleaned up shutdown ordering so media stops before Vulkan shutdown.

### Fixed
- Fixed stale copied runtime files by cleaning and recopied `sandbox/` on build.
- Fixed script path issues by loading scripts from the build output sandbox folder.
- Fixed the framebuffer resize notification path.
- Fixed image orientation issues in fullscreen textured rendering.
- Fixed fullscreen textured quad scaling behavior.
- Fixed repeated stale demo asset problems during rebuilds.
- Fixed duplicate miniaudio implementation risk by removing the old audio engine source from the build.

### Removed
- Removed hardcoded Bad Apple-specific engine logic.
- Removed Bad Apple as a required runtime demo.
- Removed demo-specific assumptions from the C++ startup flow.
- Removed the old direct script-driven media timing path from the intended v0.01 flow.

## [v0.02] – Basic 2D Runtime

### Added
- Keyboard input support.
- Mouse input support.
- Mouse position exposed to Luau.
- Basic input APIs exposed to Luau.
- Click detection.
- Window size APIs exposed to Luau.
- Runtime window title API.
- Runtime window debug title mode.
- Basic rectangle rendering.
- Basic polygon rendering.
- Basic circle rendering.
- Immediate-mode 2D shape API:
  - `proton.draw.rect`
  - `proton.draw.polygon`
  - `proton.draw.circle`
- Luau-controlled shape updates.
- Basic shape rotation through polygon rotation.
- Multiple colored shapes in one frame.
- Rect/polygon IDs with lifetime draw logging.
- Basic 2D gameplay test.
- Jump physics test.
- Ground collision test.
- Sprite manager skeleton.
- Sprite loading API:
  - `proton.sprite.load`
- Sprite handles returned to Luau.
- Sprite draw request API:
  - `proton.sprite.draw`
- Sprite draw commands collected per frame.
- Sprite draw parameters:
  - Position
  - Scale/size
  - Rotation
  - Tint/color
  - Alpha
  - Layer
- Sprite image decoding/loading.
- Sprite CPU image data storage.
- Sprite GPU texture upload.
- Sprite texture resource cache by handle.
- Textured quad rendering.
- Visible sprite rendering.
- Multiple sprite assets loaded at once.
- Multiple sprite GPU textures cached by handle.
- Multiple sprite draw commands using different textures in one frame.
- Reusing the same sprite handle for multiple draws.
- Basic layer argument support for draw APIs.
- Layer sorting inside shape rendering.
- Layer sorting inside sprite rendering.
- Cross-renderer layer ordering.
- Sprite UV orientation cleanup.
- Basic collision helpers.
- Rectangle overlap collision helper:
  - `proton.collision.rectsOverlap`
- Luau collision-driven gameplay response.
- Simple runtime object handles:
  - `proton.object.create`
  - `proton.object.setPosition`
  - `proton.object.setSize`
  - `proton.object.getX`
  - `proton.object.getY`
  - `proton.object.getWidth`
  - `proton.object.getHeight`
- Relative sprite drawing helper:
  - `proton.sprite.drawOn`
- Object-aware sprite helpers:
  - `proton.sprite.drawObject`
  - `proton.sprite.drawOnObject`
- Sprite batching improvements:
  - Consecutive same-texture sprite batching
  - Multiple sprite draw requests collapsed into fewer GPU draw batches
- Unified cross-renderer render queue.

### Changed
- Moved the 2D runtime from basic rectangle-only rendering toward a full immediate-mode 2D renderer.
- Changed sprite rendering from isolated draw calls into batched draw groups where possible.
- Changed render ordering from separate shape-first/sprite-second rendering into a unified render queue.
- Improved draw layering so shapes and sprites can be ordered together through shared layer values.
- Improved sprite rendering to support tint, alpha, rotation, scale, and multiple textures.
- Improved Luau runtime scripting so gameplay tests can be built fully from Luau.
- Improved object positioning workflow by allowing object handles to store reusable rectangle transforms.
- Improved sprite-on-object workflows with object-aware sprite helpers.
- Improved runtime debug visibility through title/debug title APIs and draw logging.

### Fixed
- Fixed mouse position mismatch issues.
- Fixed flipped Y-axis behavior in 2D gameplay tests.
- Fixed jump physics lag spike caused by excessive runtime/logging behavior.
- Fixed sprite image orientation so textures no longer render flipped.
- Fixed draw ordering limitations where sprites and shapes could not be layered together correctly.
- Fixed multiple sprite texture handling by caching GPU textures per sprite handle.
- Fixed invalid sprite draw spam by validating sprite handles before rendering.
- Fixed duplicated/incorrect sprite asset copy assumptions during sandbox testing.
- Fixed rectangle/polygon draw command ordering with layer-aware batching.
- Fixed sprite rendering so multiple image assets can render in the same frame.
- Fixed object-handle workflow so sprites can follow runtime object transforms.

### Removed
- Removed rectangle-only rendering as the practical limit of the runtime.
- Removed reliance on shape-only testing for 2D gameplay validation.
- Removed the old separate-only render ordering model in favor of a unified render queue.
- Removed temporary debug assumptions around sprite UV orientation.
- Removed temporary sprite batching/render queue debug logs after validation.