# Scripting Guide

ProtonG uses [Luau](https://luau-lang.org/) as its primary scripting language.

## Basic Usage

The entry point for scripts is `sandbox/scripts/main.luau`.

### Global `proton` table

The engine exposes functionality through the `proton` global table.

```lua
-- Set the image scale mode
proton.setImageScaleMode("fit") -- options: "fit", "fill", "stretch"

-- Media playback
proton.media.playImageSequence(path, audioPath, startFrame, endFrame, fps)
```

## Lifecycle Functions

- `update(dt: number)`: Called every frame with the delta time.

## Type Definitions

Type definitions for the engine API can be found in `sandbox/scripts/00_engine_types.d.luau`.
To get autocompletion in your editor, ensure this file is included in your Luau path.
