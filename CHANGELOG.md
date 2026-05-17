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

## [v0.02] – Planned Basic 2D Runtime

### Planned
- Keyboard input.
- Mouse input.
- Basic 2D sprite API.
- Sprite position.
- Sprite scale.
- Sprite rotation.
- Draw ordering.
- Multiple image assets.
- Luau-controlled sprite updates.
- Basic input APIs exposed to Luau.
- Simple runtime object handles.
