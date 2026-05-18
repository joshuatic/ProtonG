--!strict

--[[ Definitions file
--------------------------
Provides API definitions for
	  the Luau files
	  --------------
	  WARNING!!!!!!!
	    Deletion of
	 this file is not
	  advised, do so
	 at your own risk.
]]

type ProtonInputApi = {
	isKeyDown: (keyName: string) -> boolean,
	isKeyPressed: (keyName: string) -> boolean,
	isKeyReleased: (keyName: string) -> boolean,

	isMouseButtonDown: (buttonName: string) -> boolean,
	isMouseButtonPressed: (buttonName: string) -> boolean,
	isMouseButtonReleased: (buttonName: string) -> boolean,

	getMouseX: () -> number,
	getMouseY: () -> number,
}

type ProtonDrawApi = {
	clear: () -> (),
	rect: (
		id: string,
		x: number,
		y: number,
		width: number,
		height: number,
		r: number?,
		g: number?,
		b: number?,
		a: number?
	) -> (),
}

type ProtonMediaApi = {
	playImageSequence: (
		framePattern: string,
		audioPath: string,
		firstFrame: number,
		lastFrame: number,
		fps: number
	) -> (),
	stop: () -> (),
}

type ProtonWindowApi = {
	getWidth: () -> number,
	getHeight: () -> number,
	setTitle: (title: string) -> (),
	setDebugMode: (enabled: boolean) -> (),
}

type ProtonSpriteApi = {
	load: (path: string) -> number,

	draw: (
		id: string,
		handle: number,
		x: number,
		y: number,
		width: number,
		height: number,
		rotationDegrees: number?,
		r: number?,
		g: number?,
		b: number?,
		a: number?
	) -> (),

	drawOn: (
		id: string,
		handle: number,
		targetX: number,
		targetY: number,
		targetWidth: number,
		targetHeight: number,
		offsetX: number?,
		offsetY: number?,
		width: number?,
		height: number?,
		rotationDegrees: number?,
		r: number?,
		g: number?,
		b: number?,
		a: number?,
		layer: number?
	) -> (),

	drawObject: (
		id: string,
		handle: number,
		objectHandle: number,
		rotationDegrees: number?,
		r: number?,
		g: number?,
		b: number?,
		a: number?,
		layer: number?
	) -> (),

	drawOnObject: (
		id: string,
		handle: number,
		objectHandle: number,
		offsetX: number?,
		offsetY: number?,
		width: number?,
		height: number?,
		rotationDegrees: number?,
		r: number?,
		g: number?,
		b: number?,
		a: number?,
		layer: number?
	) -> (),
}

type ProtonCollisionApi = {
	rectsOverlap: (
	ax: number,
	ay: number,
	aw: number,
	ah: number,
	bx: number,
	by: number,
	bw: number,
	bh: number
	) -> boolean,
}

type ProtonObjectApi = {
	create: (
	name: string,
	x: number?,
	y: number?,
	width: number?,
	height: number?
	) -> number,

	setPosition: (handle: number, x: number, y: number) -> (),
	setSize: (handle: number, width: number, height: number) -> (),

	getX: (handle: number) -> number,
	getY: (handle: number) -> number,
	getWidth: (handle: number) -> number,
	getHeight: (handle: number) -> number,
}

type ProtonApi = {
	setClearColor: (r: number, g: number, b: number, a: number?) -> (),
	renderImage: (path: string) -> (),
	playAudio: (path: string) -> (),
	getAudioTime: () -> number,
	setImageScaleMode: (mode: "stretch" | "fit" | "fill" | "native") -> (),

	draw: ProtonDrawApi,
	input: ProtonInputApi,
	media: ProtonMediaApi,
	window: ProtonWindowApi,
	sprite: ProtonSpriteApi,
	collision: ProtonCollisionApi,
	object: ProtonObjectApi,
}

declare proton: ProtonApi