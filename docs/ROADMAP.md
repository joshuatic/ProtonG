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

**Status:** Feature-complete / final validation

Goal: add enough 2D runtime functionality to build simple interactive scenes from Luau, including input, immediate-mode rendering, sprites, object handles, collision helpers, batching, and shared render ordering.

### Completed
- [x] Keyboard input
- [x] Mouse input
- [x] Mouse position exposed to Luau
- [x] Basic Luau input API
- [x] Window size API
- [x] Runtime window title API
- [x] Runtime debug title mode
- [x] Basic rectangle rendering
- [x] Basic polygon rendering
- [x] Basic circle rendering
- [x] Immediate-mode 2D draw API
    - [x] `proton.draw.rect`
    - [x] `proton.draw.polygon`
    - [x] `proton.draw.circle`
- [x] Sprite manager
- [x] Sprite loading API
    - [x] `proton.sprite.load`
- [x] Sprite handles
- [x] Sprite drawing API
    - [x] `proton.sprite.draw`
- [x] Sprite properties
    - [x] Sprite position
    - [x] Sprite scale/size
    - [x] Sprite rotation
    - [x] Sprite tint/color
    - [x] Sprite alpha
- [x] Multiple image assets
- [x] Sprite CPU image data storage
- [x] Sprite GPU texture upload
- [x] Sprite texture cache by handle
- [x] Textured quad rendering
- [x] Sprite UV orientation cleanup
- [x] Basic draw ordering/layers
- [x] Cross-renderer layer ordering
- [x] Unified cross-renderer render queue
- [x] Consecutive same-texture sprite batching
- [x] Simple runtime object handles
    - [x] `proton.object.create`
    - [x] `proton.object.setPosition`
    - [x] `proton.object.setSize`
    - [x] `proton.object.getX`
    - [x] `proton.object.getY`
    - [x] `proton.object.getWidth`
    - [x] `proton.object.getHeight`
- [x] Relative sprite drawing helper
    - [x] `proton.sprite.drawOn`
- [x] Object-aware sprite helpers
    - [x] `proton.sprite.drawObject`
    - [x] `proton.sprite.drawOnObject`
- [x] Basic collision helpers
    - [x] `proton.collision.rectsOverlap`
- [x] Luau-driven gameplay test
    - [x] Jump physics test
    - [x] Ground collision test
- [x] Final v0.02 sanity test scene
- [x] CI documentation/update work
- [x] Final cleanup before tagging
    - [x] Remove temporary local test spam from sandbox script if desired
    - [x] Confirm no render queue / batching debug spam remains
    - [x] Confirm CI passes on Ubuntu and Windows
    - [x] Push final v0.02 docs and workflow fixes
    - [x] Tag or mark v0.02 as complete

### Notes

v0.02 started as a basic 2D runtime milestone, but it now includes enough functionality to behave like a small Luau-driven 2D engine slice. The runtime can render shapes and sprites, load multiple textures, sort shapes and sprites together by layer, batch compatible sprite draws, create simple object handles, and run collision-driven gameplay tests.

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