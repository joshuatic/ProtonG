--!strict

print("Proton G sandbox script loaded.")

export type ImageScaleMode =
"stretch"
| "fit"
| "fill"
| "native"

export type ProtonMediaApi = {
	playImageSequence: (
	framePattern: string,
	audioPath: string,
	firstFrame: number,
	lastFrame: number,
	fps: number
	) -> (),

	stop: () -> (),
}

export type ProtonApi = {
	setClearColor: (r: number, g: number, b: number, a: number?) -> (),
	renderImage: (path: string) -> (),
	playAudio: (path: string) -> (),
	setImageScaleMode: (mode: ImageScaleMode) -> (),

	media: ProtonMediaApi,
}

local protonApi = (_G.proton :: any) :: ProtonApi

protonApi.setImageScaleMode("fit")

-- Example only. Put your own asset folder here.
-- protonApi.media.playImageSequence(
-- 	"sandbox/assets/demo/frames/output_%04d.jpg",
-- 	"sandbox/assets/demo/audio/audio.wav",
-- 	1,
-- 	300,
-- 	30
-- )

function _update(dt: number)
-- This function is intentionally empty for now.
-- Proton G can call this every frame later.
end