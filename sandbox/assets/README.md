# Assets Directory

Place Proton G sandbox assets here.

This folder contains assets used by sandbox scripts, demos, and runtime validation tests.

Assets in this folder may be copied into the build output sandbox folder during the build process.

## Current v0.02 Usage

v0.02 uses this folder for sprite and image loading tests.

Sprites can be loaded from Luau with:

```lua
proton.sprite.load("sandbox/assets/download.jpg")
```

### Common Current Test Assets

- `download.jpg`
- `second.jpg`
- `uv_test.png`

## Recommended Structure

- `demo/`: Demo-specific assets.
- `demo/frames/`: Image sequence frames.
- `demo/audio/`: Demo audio files.
- `fonts/`: UI or future text rendering fonts.
- `textures/`: Shared texture assets.
- `sprites/`: 2D sprite assets.
- `tests/`: Small validation assets for runtime tests.

## Asset Rules

- Keep committed assets small.
- Prefer PNG or JPG for sprite tests.
- Use clear filenames.
- Keep paths stable so Luau scripts do not break.
- Avoid committing large temporary images.
- Avoid committing generated build output.
- Avoid committing personal/random downloaded files unless they are intentionally used as test assets.
- Do not place compiled binaries or build artifacts here.

## Current v0.02 Asset Test Coverage

Assets in this folder are used to test:

- Sprite decoding.
- CPU image data storage.
- GPU texture upload.
- Texture cache by sprite handle.
- Multiple loaded sprite assets.
- Sprite tint/color.
- Sprite alpha.
- Sprite rotation.
- Sprite scale/size.
- Object-aware sprite drawing.
- Sprite batching.
- UV orientation correctness.

## Path Examples

Use sandbox-relative paths from Luau:

- `sandbox/assets/download.jpg`
- `sandbox/assets/second.jpg`
- `sandbox/assets/uv_test.png`

### Example

```lua
local playerSprite = proton.sprite.load("sandbox/assets/download.jpg")
```

## Notes

- The source `sandbox/assets/` folder is copied into the build output sandbox folder.
- Runtime scripts usually load from the copied build-output path indirectly through the same sandbox-relative path.