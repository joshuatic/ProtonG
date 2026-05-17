# Proton G Roadmap

This roadmap tracks the planned development milestones for Proton G.

Proton G is currently an early Luau-first C++/Vulkan runtime. The first goal is to build a stable runtime foundation before adding full engine/editor features.

---

## v0.01 – Asset Runtime

**Status:** Current milestone

Goal: prove that Proton G can boot, load scripts, display images, and support basic media playback.

- [x] GLFW window creation
- [x] Vulkan instance creation
- [x] Vulkan surface creation
- [x] Vulkan physical device selection
- [x] Vulkan logical device creation
- [x] Vulkan swapchain creation
- [x] Vulkan render pass
- [x] Vulkan framebuffers
- [x] Vulkan command buffers
- [x] Vulkan sync objects
- [x] Fullscreen textured image rendering
- [x] Image loading
- [x] Image scale modes
    - [x] Stretch
    - [x] Fit
    - [x] Fill
    - [x] Native
- [x] Luau runtime integration
- [x] Sandbox script loading
- [x] Luau `update(dt)` loop
- [x] Runtime stats logging
- [x] FPS window title
- [x] Sandbox folder copy on build
- [x] Generic MediaPlayer foundation
- [x] Basic audio decoding/playback path
- [x] Public repo cleanup

---

## v0.02 – Basic 2D Runtime

**Status:** Planned

Goal: add enough 2D functionality to make simple interactive scenes from Luau.

- [ ] Keyboard input
- [ ] Mouse input
- [ ] Luau input API
- [ ] Basic sprite rendering
- [ ] Sprite position
- [ ] Sprite scale
- [ ] Sprite rotation
- [ ] Sprite draw order
- [ ] Multiple image assets
- [ ] Basic asset handles
- [ ] Clearer runtime object ownership
- [ ] Simple 2D demo scene

---

## v0.03 – Scene Runtime

**Status:** Planned

Goal: move from one-off rendering into a basic scene/object model.

- [ ] Scene object registry
- [ ] Object handles exposed to Luau
- [ ] Basic transform components
- [ ] Simple camera
- [ ] Asset manager
- [ ] Multiple sprites on screen
- [ ] Runtime object creation from Luau
- [ ] Runtime object deletion from Luau
- [ ] Better logging for script/runtime errors

---

## v0.04 - Scripting Improvements

**Status:** Planned

Goal: make Luau feel like the main way to control Proton G.

- [ ] Expanded `proton` API
- [ ] Better Luau type declarations
- [ ] Script error reporting improvements
- [ ] Script reload support
- [ ] Optional script module loading
- [ ] Cleaner sandbox API structure
- [ ] Better Luau workflow notes

---

## v0.05 – Media Runtime Improvements

**Status:** Planned

Goal: improve the media system after the core runtime is stable.

- [ ] Generic Luau media APIs
- [ ] Image sequence playback from Luau
- [ ] Audio playback from Luau
- [ ] Audio stop/pause/resume
- [ ] Volume control
- [ ] Looping audio
- [ ] Better media diagnostics
- [ ] Optional video decoding research

---

## v0.06 – Performance Pass

**Status:** Planned

Goal: reduce unnecessary work and prepare the runtime for larger scenes.

- [ ] Texture caching
- [ ] Asset reuse
- [ ] Reduced per-frame allocations
- [ ] Better frame pacing
- [ ] Optional VSync configuration
- [ ] Shader cleanup
- [ ] Render path cleanup
- [ ] Memory usage diagnostics

---

## Future / Long-Term Ideas

These are not locked to a version yet.

- [ ] Editor/runtime split
- [ ] File browser
- [ ] Asset preview panel
- [ ] Runtime console
- [ ] Project system
- [ ] Hot reload
- [ ] 3D rendering
- [ ] Luau-first game framework
- [ ] Proton G IDE integration