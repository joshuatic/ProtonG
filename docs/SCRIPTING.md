# Scripting Guide

Proton G uses [Luau](https://luau-lang.org/) as its primary scripting language.

The current v0.02 scripting runtime focuses on immediate-mode 2D rendering, sprites, input, simple runtime object handles, and basic collision helpers.

## Basic Usage

The default script entry point is `sandbox/scripts/main.luau`.

The engine exposes functionality through the global `proton` table.

Scripts usually follow this pattern:
1. Configure the window.
2. Load assets.
3. Create runtime objects.
4. Implement `update(dt: number)`.
5. Draw shapes/sprites every frame.

## Lifecycle Functions

### `update(dt: number)`

Called every frame. `dt` is the frame delta time in seconds.

Use `dt` for frame-rate-independent movement, gravity, rotation, animation, and timers.

Example logic:
- Move with `speed * dt`.
- Apply gravity with `velocity += gravity * dt`.
- Rotate with `rotation += degreesPerSecond * dt`.

## Global `proton` Table

The `proton` table contains the runtime APIs available to Luau scripts.

### Clear Color

`proton.setClearColor(r, g, b, a)`

Sets the renderer clear color. Values are usually between `0.0` and `1.0`.

Example:
```lua
proton.setClearColor(0.05, 0.08, 0.16, 1.0)
```

### Window API

Available through `proton.window`.

- `proton.window.setTitle(title)`: Changes the runtime window title.
  ```lua
  proton.window.setTitle("My Proton G Game")
  ```
- `proton.window.setDebugMode(enabled)`: Enables or disables debug information in the window title.
  ```lua
  proton.window.setDebugMode(true)
  ```
- `proton.window.getWidth()`: Returns the current window width.
- `proton.window.getHeight()`: Returns the current window height.

Common use:
```lua
local screenW = proton.window.getWidth()
local screenH = proton.window.getHeight()
```

### Input API

Available through `proton.input`.

- `proton.input.isKeyDown(key)`: Returns `true` while a key is being held.
  Example keys: `"a"`, `"d"`, `"left"`, `"right"`, `"space"`.
- `proton.input.isKeyPressed(key)`: Returns `true` on the frame a key was pressed. Good for jumping, actions, and one-shot events.
- `proton.input.isMouseButtonDown(button)`: Returns `true` while a mouse button is held (e.g., `"left"`).
- `proton.input.isMouseButtonPressed(button)`: Returns `true` on the frame a mouse button was pressed.
- `proton.input.getMouseX()`: Returns the current mouse X position.
- `proton.input.getMouseY()`: Returns the current mouse Y position.

### Draw API

Available through `proton.draw`.

The draw API is immediate-mode. This means shapes must be submitted every frame inside `update(dt)`.

- `proton.draw.rect(id, x, y, width, height, r, g, b, a, layer)`: Draws a rectangle.
    - `id`: stable draw ID.
    - `x, y`: top-left position.
    - `width, height`: rectangle size.
    - `r, g, b, a`: color.
    - `layer`: render layer.
  Example:
  ```lua
  proton.draw.rect("ground", 0, 580, 1280, 140, 0.2, 0.2, 0.2, 1, 0)
  ```

- `proton.draw.circle(id, centerX, centerY, radius, r, g, b, a, layer)`: Draws a circle. Internally, circles are rendered as polygons with many points.
  Example:
  ```lua
  proton.draw.circle("orb", 760, 494, 34, 1, 0.8, 0.2, 0.8, 30)
  ```

- `proton.draw.polygon(id, centerX, centerY, radius, points, rotationDegrees, r, g, b, a, layer)`: Draws a regular polygon.
    - `points = 3` creates a triangle.
    - `points = 6` creates a hexagon.
  Example:
  ```lua
  proton.draw.polygon("spike", 520, 540, 40, 3, -90, 1, 1, 1, 1, 10)
  ```

### Sprite API

Available through `proton.sprite`. Sprites are loaded from the sandbox asset folder and rendered as textured quads.

- `proton.sprite.load(path)`: Loads a sprite image and returns a sprite handle.
  Example:
  ```lua
  local playerSprite = proton.sprite.load("sandbox/assets/download.jpg")
  ```
  A valid sprite handle should be greater than `0`.

- `proton.sprite.draw(id, handle, x, y, width, height, rotationDegrees, r, g, b, a, layer)`: Draws a sprite.
  Example:
  ```lua
  proton.sprite.draw("player", playerSprite, 160, 484, 96, 96, 0, 1, 1, 1, 1, 20)
  ```

- `proton.sprite.drawOn(id, handle, targetX, targetY, targetWidth, targetHeight, offsetX, offsetY, width, height, rotationDegrees, r, g, b, a, layer)`: Draws a sprite relative to a target rectangle. Useful for placing a sprite inside or on top of another rectangle-like object.
  Example:
  ```lua
  proton.sprite.drawOn("face", faceSprite, playerX, playerY, playerW, playerH, 24, 24, 48, 48, 0, 1, 1, 1, 0.85, 21)
  ```

- `proton.sprite.drawObject(id, handle, objectHandle, rotationDegrees, r, g, b, a, layer)`: Draws a sprite using a runtime object’s position and size.
  Example:
  ```lua
  proton.sprite.drawObject("player_body", playerSprite, player, 0, 1, 1, 1, 1, 20)
  ```

- `proton.sprite.drawOnObject(id, handle, objectHandle, offsetX, offsetY, width, height, rotationDegrees, r, g, b, a, layer)`: Draws a sprite relative to a runtime object.
  Example:
  ```lua
  proton.sprite.drawOnObject("player_face", faceSprite, player, 24, 24, 48, 48, 0, 1, 1, 1, 0.85, 21)
  ```

### Object API

Available through `proton.object`. Runtime objects are simple rectangle transform handles. They store: `Name`, `X position`, `Y position`, `Width`, and `Height`.

They are not full Roblox-style instances yet. They are lightweight v0.02 runtime handles.

- `proton.object.create(name, x, y, width, height)`: Creates an object and returns an object handle.
  Example:
  ```lua
  local player = proton.object.create("player", 160, 0, 96, 96)
  ```
- `proton.object.setPosition(handle, x, y)`: Updates an object’s position.
- `proton.object.setSize(handle, width, height)`: Updates an object’s size.
- `proton.object.getX(handle)`, `proton.object.getY(handle)`, `proton.object.getWidth(handle)`, `proton.object.getHeight(handle)`: Getters for object properties.

### Collision API

Available through `proton.collision`.

- `proton.collision.rectsOverlap(ax, ay, aw, ah, bx, by, bw, bh)`: Returns `true` if two rectangles overlap.
  Example:
  ```lua
  local hit = proton.collision.rectsOverlap(playerX, playerY, playerW, playerH, spikeX, spikeY, spikeW, spikeH)
  ```

Useful for:
- Player vs hazard checks.
- Player vs enemy checks.
- Pickup detection.
- Simple platformer tests.

## Layers and Render Order

Lower layer values render first. Higher layer values render later.

Example:
- Layer 0: background and ground.
- Layer 10: hazards.
- Layer 20: player/enemy sprites.
- Layer 30: foreground effects.
- Layer 100: screen flash or overlay.

Shapes and sprites are sorted through a unified cross-renderer render queue. When shapes and sprites share the same layer, shape batches currently render before sprite batches.

## Sprite Batching

Sprite rendering supports consecutive same-texture batching. If multiple sprite draw calls use the same sprite handle on the same layer without interruption, Proton G can collapse them into fewer GPU draw batches. This improves rendering without changing script behavior.

## Asset Paths

Sprite assets are loaded from sandbox paths.
Examples:
- `sandbox/assets/download.jpg`
- `sandbox/assets/second.jpg`
- `sandbox/assets/uv_test.png`

Keep test assets small and avoid committing large temporary images.

## Type Definitions

Type definitions for the engine API are located at: `sandbox/scripts/00_engine_types.d.luau`.
For editor autocomplete, make sure this file is included in your Luau path.

## Current v0.02 Status

v0.02 is feature-complete as the **Basic 2D Runtime** milestone.

It includes:
- Input.
- Window APIs.
- Shape rendering.
- Sprite rendering.
- Sprite loading.
- Sprite tint/alpha/rotation/scale/layers.
- Runtime object handles.
- Object-aware sprite helpers.
- Rectangle collision helper.
- Sprite batching.
- Unified render queue.