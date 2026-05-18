--!strict

print("Proton G v0.02 final sanity test loaded.")

proton.window.setTitle("Proton G v0.02 Final Test")
proton.window.setDebugMode(true)

proton.setClearColor(0.05, 0.08, 0.16, 1.0)

local playerSprite = proton.sprite.load("sandbox/assets/download.jpg")
local faceSprite = proton.sprite.load("sandbox/assets/second.jpg")

print("playerSprite handle", playerSprite)
print("faceSprite handle", faceSprite)

local player = proton.object.create("player", 160, 0, 96, 96)
local enemy = proton.object.create("enemy", 620, 0, 72, 72)

local velocityY = 0
local gravity = 1800
local jumpPower = -720
local grounded = false

local bodyRotation = 0
local enemyRotation = 0

local spikeX = 520
local spikeSize = 80

function update(dt: number)
	local screenW = proton.window.getWidth()
	local screenH = proton.window.getHeight()

	local groundH = 140
	local groundY = screenH - groundH

	local playerX = proton.object.getX(player)
	local playerY = proton.object.getY(player)
	local playerW = proton.object.getWidth(player)
	local playerH = proton.object.getHeight(player)

	local enemyX = proton.object.getX(enemy)
	local enemyY = groundY - proton.object.getHeight(enemy)
	local enemyW = proton.object.getWidth(enemy)
	local enemyH = proton.object.getHeight(enemy)

	bodyRotation += 90 * dt
	enemyRotation -= 120 * dt

	if proton.input.isKeyDown("d") or proton.input.isKeyDown("right") then
		playerX += 300 * dt
	end

	if proton.input.isKeyDown("a") or proton.input.isKeyDown("left") then
		playerX -= 300 * dt
	end

	if playerX < 0 then
		playerX = 0
	end

	if playerX + playerW > screenW then
		playerX = screenW - playerW
	end

	if (proton.input.isKeyPressed("space") or proton.input.isMouseButtonPressed("left")) and grounded then
		velocityY = jumpPower
		grounded = false
		print("Final test jump")
	end

	velocityY += gravity * dt
	playerY += velocityY * dt

	if playerY + playerH >= groundY then
		playerY = groundY - playerH
		velocityY = 0
		grounded = true
	end

	proton.object.setPosition(player, playerX, playerY)
	proton.object.setPosition(enemy, enemyX, enemyY)

	local spikeBoxX = spikeX - spikeSize * 0.5
	local spikeBoxY = groundY - spikeSize
	local spikeBoxW = spikeSize
	local spikeBoxH = spikeSize

	local hitSpike = proton.collision.rectsOverlap(
		playerX,
		playerY,
		playerW,
		playerH,
		spikeBoxX,
		spikeBoxY,
		spikeBoxW,
		spikeBoxH
	)

	local hitEnemy = proton.collision.rectsOverlap(
		playerX,
		playerY,
		playerW,
		playerH,
		enemyX,
		enemyY,
		enemyW,
		enemyH
	)

	local danger = hitSpike or hitEnemy

	-- Layer 0: background/ground
	proton.draw.rect("background_panel", 80, 80, screenW - 160, 360, 0.08, 0.1, 0.18, 1, 0)
	proton.draw.rect("ground", 0, groundY, screenW, groundH, 0.2, 0.2, 0.2, 1, 0)

	-- Layer 5: decorative shapes behind gameplay
	proton.draw.circle("background_circle", 180, groundY - 260, 80, 0.1, 0.35, 0.8, 0.55, 5)
	proton.draw.polygon("background_hex", 1040, groundY - 260, 64, 6, bodyRotation, 0.2, 0.8, 1, 0.55, 5)

	-- Layer 10: hazards
	proton.draw.polygon(
		"spike",
		spikeX,
		groundY - spikeSize * 0.5,
		spikeSize * 0.5,
		3,
		-90,
		1,
		1,
		1,
		1,
		10
	)

	-- Layer 15: repeated sprites for batching test
	for i = 0, 4 do
		proton.sprite.draw(
			"batch_player_" .. tostring(i),
			playerSprite,
			120 + i * 70,
			groundY - 220,
			48,
			48,
			bodyRotation,
			1,
			1,
			1,
			0.75,
			15
		)
	end

	for i = 0, 2 do
		proton.sprite.draw(
			"batch_face_" .. tostring(i),
			faceSprite,
			520 + i * 70,
			groundY - 220,
			48,
			48,
			-enemyRotation,
			1,
			1,
			1,
			0.75,
			15
		)
	end

	-- Layer 20/21: object-aware sprite rendering
	proton.sprite.drawObject(
		"player_body",
		playerSprite,
		player,
		bodyRotation,
		1,
		danger and 0.35 or 1,
		danger and 0.35 or 1,
		1,
		20
	)

	proton.sprite.drawOnObject(
		"player_face",
		faceSprite,
		player,
		24,
		24,
		48,
		48,
		0,
		1,
		1,
		1,
		0.85,
		21
	)

	proton.sprite.drawObject(
		"enemy_body",
		faceSprite,
		enemy,
		enemyRotation,
		1,
		1,
		1,
		0.9,
		20
	)

	-- Layer 30: shape over sprites to prove cross-renderer ordering
	proton.draw.circle("foreground_orb", 760, groundY - 86, 34, 1, 0.8, 0.2, 0.8, 30)

	-- Layer 100: collision response
	if danger then
		proton.draw.rect("danger_flash", 0, 0, screenW, screenH, 1, 0, 0, 0.25, 100)
	end
end