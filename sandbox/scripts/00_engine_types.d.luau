--!strict

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

local proton = (_G.proton :: any) :: ProtonApi

return proton