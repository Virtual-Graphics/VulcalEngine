-- Last Three Minutes. All gameplay stays local to this script instance.
local config = {
    step = 1 / 120, max_steps = 12, duration = 180, half_size = 10,
    player_radius = 0.38, player_speed = 6, health = 3, hurt_time = 1,
    dash_speed = 20, dash_time = 0.15, dash_cooldown = 1,
    bullet_speed = 24, bullet_radius = 0.12, bullet_life = 1.25, fire_interval = 0.14,
    max_enemies = 90, max_bullets = 48, max_particles = 160,
}
local color = {
    background = { 0.018, 0.025, 0.045, 1 }, board = { 0.035, 0.05, 0.08, 1 },
    grid = { 0.055, 0.075, 0.11, 1 }, wall = { 0.18, 0.3, 0.43, 1 },
    player = { 0.2, 0.95, 0.85, 1 }, bullet = { 1, 0.92, 0.5, 1 },
    normal = { 1, 0.35, 0.4, 1 }, fast = { 1, 0.65, 0.22, 1 },
    text = { 0.8, 0.88, 0.95, 1 }, muted = { 0.3, 0.42, 0.55, 1 },
    white = { 1, 1, 1, 1 }, panel = { 0.025, 0.04, 0.07, 0.97 },
}
local layer = { board = -10, grid = -9, wall = -8, enemy = 1, bullet = 2, player = 3,
                effect = 4, hud = 10, panel = 20, prompt = 21 }

-- A tiny project-local 3x5 alphabet, not a font or UI subsystem.
local glyph = {
    A = "010101111101101", B = "110101110101110", C = "011100100100011",
    D = "110101101101110", E = "111100110100111", F = "111100110100100",
    G = "011100101101011", H = "101101111101101", I = "111010010010111",
    J = "001001001101010", K = "101101110101101", L = "100100100100111",
    M = "101111111101101", N = "101111111111101", O = "010101101101010",
    P = "110101110100100", Q = "010101101111011", R = "110101110101101",
    S = "011100010001110", T = "111010010010010", U = "101101101101111",
    V = "101101101101010", W = "101101111111101", X = "101101010101101",
    Y = "101101010010010", Z = "111001010100111",
    ["0"] = "111101101101111", ["1"] = "010110010010111", ["2"] = "110001010100111",
    ["3"] = "110001010001110", ["4"] = "101101111001001", ["5"] = "111100110001110",
    ["6"] = "011100111101111", ["7"] = "111001010010010", ["8"] = "111101111101111",
    ["9"] = "111101111001110", [":"] = "000010000010000",
}
local game = {}
local owned, hud, prompts = {}, {}, {}
local camera, environment
local camera_base = { 0, 0, 10 }
local directions = {
    { key = Key.Up, x = 0, y = 1 }, { key = Key.Down, x = 0, y = -1 },
    { key = Key.Left, x = -1, y = 0 }, { key = Key.Right, x = 1, y = 0 },
}

local function clamp(value, low, high)
    return math.max(low, math.min(value, high))
end

local function normalized(x, y)
    local length = math.sqrt(x * x + y * y)
    if length > 0 then return x / length, y / length end
    return 0, 0
end

local function rectangle(name, x, y, w, h, tint, order, persistent)
    local item = scene:create_entity(name)
    local sprite = item:add_sprite()
    sprite.color = tint
    sprite.sorting_order = order
    item:set_translation(x, y, 0)
    item:set_scale(w, h, 1)
    if persistent then owned[#owned + 1] = item end
    return item
end

local function destroy(item)
    if item and item.valid then item:destroy() end
end

local function show(group, visible)
    for _, item in ipairs(group) do item.sprite.visible = visible end
end

local function text(group, value, x, y, pixel, tint, order)
    local left = x - (#value * 4 - 1) * pixel / 2
    for index = 1, #value do
        local pattern = glyph[value:sub(index, index)]
        if pattern then
            for bit = 1, 15 do
                if pattern:sub(bit, bit) == "1" then
                    group[#group + 1] = rectangle("Letter", left + ((index - 1) * 4 + (bit - 1) % 3 + 0.5) * pixel,
                        y + (2 - math.floor((bit - 1) / 3)) * pixel,
                        pixel * 0.88, pixel * 0.88, tint, order, true)
                end
            end
        end
    end
end

local function number_display(x, y, digits)
    local display = { digits = {}, value = -1 }
    for index = 1, digits do
        local pixels = {}
        local left = x + (index - (digits + 1) / 2) * 0.72
        for bit = 1, 15 do
            pixels[bit] = rectangle("Digit", left + ((bit - 1) % 3 - 1) * 0.17,
                y + (2 - math.floor((bit - 1) / 3)) * 0.17, 0.15, 0.15, color.text, layer.hud, true)
        end
        display.digits[index] = pixels
    end
    return display
end

local function set_number(display, value)
    value = clamp(math.floor(value), 0, 10 ^ #display.digits - 1)
    if display.value == value then return end
    display.value = value
    local formatted = string.format("%0" .. #display.digits .. "d", value)
    for index, pixels in ipairs(display.digits) do
        local pattern = glyph[formatted:sub(index, index)]
        for bit, item in ipairs(pixels) do item.sprite.visible = pattern:sub(bit, bit) == "1" end
    end
end

local function create_stage()
    rectangle("Board", 0, 0, 20, 20, color.board, layer.board, true)
    for i = -9, 9 do
        rectangle("Grid", i, 0, 0.018, 20, color.grid, layer.grid, true)
        rectangle("Grid", 0, i, 20, 0.018, color.grid, layer.grid, true)
    end
    rectangle("WallTop", 0, 10.15, 20.6, 0.3, color.wall, layer.wall, true)
    rectangle("WallBottom", 0, -10.15, 20.6, 0.3, color.wall, layer.wall, true)
    rectangle("WallLeft", -10.15, 0, 0.3, 20, color.wall, layer.wall, true)
    rectangle("WallRight", 10.15, 0, 0.3, 20, color.wall, layer.wall, true)
    local labels = {}
    text(labels, "LAST THREE MINUTES", 0, 11.5, 0.19, color.player, layer.hud)
    text(labels, "WASD MOVE   ARROWS FIRE   SPACE DASH", 0, -11.5, 0.13, color.text, layer.hud)
    text(labels, "LIFE", -14, 7, 0.18, color.text, layer.hud)
    text(labels, "DASH", -14, 3, 0.18, color.text, layer.hud)
    text(labels, "TIME", 14, 7, 0.18, color.text, layer.hud)
    text(labels, "KILLS", 14, 2, 0.18, color.text, layer.hud)
    hud.hearts = {}
    for i = 1, config.health do
        hud.hearts[i] = rectangle("Life", -15.2 + (i - 1) * 1.2, 5.5, 0.85, 0.85,
            color.player, layer.hud, true)
    end
    rectangle("DashTrack", -14, 1.5, 4, 0.28, color.muted, layer.hud, true)
    hud.dash = rectangle("DashBar", -14, 1.5, 4, 0.28, color.player, layer.hud, true)
    rectangle("TimeTrack", 14, 4.1, 4, 0.28, color.muted, layer.hud, true)
    hud.time = rectangle("TimeBar", 14, 4.1, 4, 0.28, color.player, layer.hud, true)
    hud.seconds = number_display(14, 5.5, 3)
    hud.kills = number_display(14, 0.5, 4)

    prompts.panel = { rectangle("PromptPanel", 0, 0, 16, 7, color.panel, layer.panel, true) }
    prompts.waiting, prompts.won, prompts.lost, prompts.enter = {}, {}, {}, {}
    text(prompts.waiting, "SURVIVE 180 SECONDS", 0, 1.4, 0.18, color.player, layer.prompt)
    text(prompts.waiting, "THREE HITS AND YOU ARE OUT", 0, 0, 0.12, color.text, layer.prompt)
    text(prompts.won, "YOU SURVIVED", 0, 1.4, 0.22, color.player, layer.prompt)
    text(prompts.lost, "ARENA LOST", 0, 1.4, 0.22, color.normal, layer.prompt)
    text(prompts.enter, "PRESS ENTER", 0, -1.6, 0.18, color.text, layer.prompt)
end

local function update_hud()
    if hud.health ~= game.health then
        hud.health = game.health
        for i, heart in ipairs(hud.hearts) do
            heart.sprite.color = i <= game.health and color.player or color.muted
        end
    end
    local ready = 1 - clamp(game.dash_cooldown / config.dash_cooldown, 0, 1)
    hud.dash:set_scale(math.max(0.001, 4 * ready), 0.28, 1)
    hud.dash:set_translation(-16 + 2 * ready, 1.5, 0)
    local remaining = clamp(1 - game.elapsed / config.duration, 0, 1)
    hud.time:set_scale(math.max(0.001, 4 * remaining), 0.28, 1)
    hud.time:set_translation(12 + 2 * remaining, 4.1, 0)
    set_number(hud.seconds, math.ceil(math.max(0, config.duration - game.elapsed - 1e-7)))
    set_number(hud.kills, game.kills)
    if hud.state ~= game.state then
        hud.state = game.state
        show(prompts.panel, game.state ~= "playing")
        show(prompts.enter, game.state ~= "playing")
        show(prompts.waiting, game.state == "waiting")
        show(prompts.won, game.state == "won")
        show(prompts.lost, game.state == "lost")
    end
end

local function clear_dynamic()
    destroy(game.player)
    for _, list in ipairs({ game.enemies or {}, game.bullets or {}, game.particles or {} }) do
        for _, item in ipairs(list) do destroy(item.entity) end
    end
end

local function reset(state)
    clear_dynamic()
    game = {
        state = state, elapsed = 0, health = config.health, kills = 0, accumulator = 0,
        x = 0, y = 0, move_x = 0, move_y = 1, dash_x = 0, dash_y = 1,
        dash_left = 0, dash_cooldown = 0, dash_requested = false, hurt_left = 0,
        fire_left = 0, spawn_left = 0.8, trail_left = 0, shake_left = 0, shake_strength = 0,
        enemies = {}, bullets = {}, particles = {}, aim_order = {},
    }
    game.player = rectangle("Player", 0, 0, 0.76, 0.76, color.player, layer.player)
    if camera then camera:set_translation(table.unpack(camera_base)) end
    if environment then environment.clear_color = color.background end
    update_hud()
end

local function particle(x, y, tint, vx, vy, size, life)
    if #game.particles >= config.max_particles then return end
    game.particles[#game.particles + 1] = {
        entity = rectangle("Particle", x, y, size, size, tint, layer.effect),
        x = x, y = y, vx = vx, vy = vy, size = size, left = life, life = life,
    }
end

local function burst(x, y, tint, count)
    for _ = 1, count do
        local angle, speed = math.random() * math.pi * 2, 1.5 + math.random() * 3
        particle(x, y, tint, math.cos(angle) * speed, math.sin(angle) * speed, 0.13, 0.32)
    end
end

local function shake(strength, duration)
    game.shake_strength = math.max(game.shake_strength, strength)
    game.shake_left = math.max(game.shake_left, duration)
end

local function finish(state)
    game.state = state
    game.accumulator = 0
    burst(game.x, game.y, state == "won" and color.player or color.normal, 24)
    shake(0.2, 0.25)
    Log.info(string.format("Arena %s: %d kills, %.1f seconds", state, game.kills, game.elapsed))
end

local function read_input()
    -- The bindings expose frame transitions, not within-frame event timestamps.
    -- Simultaneous presses use this stable Up/Down/Left/Right tie order.
    for _, direction in ipairs(directions) do
        if Input.is_key_pressed(direction.key) then
            for i = #game.aim_order, 1, -1 do
                if game.aim_order[i] == direction then table.remove(game.aim_order, i) end
            end
            game.aim_order[#game.aim_order + 1] = direction
        end
    end
    for i = #game.aim_order, 1, -1 do
        if not Input.is_key_down(game.aim_order[i].key) then table.remove(game.aim_order, i) end
    end
    -- Also recognize keys held when Play starts or the window regains focus.
    for _, direction in ipairs(directions) do
        local present = false
        for _, held in ipairs(game.aim_order) do present = present or held == direction end
        if Input.is_key_down(direction.key) and not present then table.insert(game.aim_order, 1, direction) end
    end
    if Input.is_key_pressed(Key.Space) then game.dash_requested = true end
end

local function update_player(dt)
    game.dash_cooldown = math.max(0, game.dash_cooldown - dt)
    game.hurt_left = math.max(0, game.hurt_left - dt)
    local mx, my = normalized(Input.key_axis(Key.A, Key.D), Input.key_axis(Key.S, Key.W))
    if mx ~= 0 or my ~= 0 then game.move_x, game.move_y = mx, my end
    if game.dash_requested and game.dash_cooldown <= 0 then
        game.dash_left, game.dash_cooldown = config.dash_time, config.dash_cooldown
        game.dash_x, game.dash_y = game.move_x, game.move_y
    end
    game.dash_requested = false
    -- Split the last dash step so the duration is independent of rounding.
    local dash_dt = math.min(dt, game.dash_left)
    local move_dt = dt - dash_dt
    local limit = config.half_size - config.player_radius
    game.x = clamp(game.x + game.dash_x * config.dash_speed * dash_dt + mx * config.player_speed * move_dt, -limit, limit)
    game.y = clamp(game.y + game.dash_y * config.dash_speed * dash_dt + my * config.player_speed * move_dt, -limit, limit)
    game.dashing = dash_dt > 0
    game.dash_left = math.max(0, game.dash_left - dt)
    if game.dashing then
        game.trail_left = game.trail_left - dt
        if game.trail_left <= 0 then
            particle(game.x, game.y, color.player, 0, 0, 0.6, 0.15)
            game.trail_left = 0.025
        end
    end
    game.player:set_translation(game.x, game.y, 0)
end

local function spawn_enemy()
    if #game.enemies >= config.max_enemies then return end
    local edge = config.half_size - 0.6
    local side, offset = math.random(1, 4), (math.random() * 2 - 1) * edge
    local x, y
    if side == 1 then x, y = -edge, offset
    elseif side == 2 then x, y = edge, offset
    elseif side == 3 then x, y = offset, -edge
    else x, y = offset, edge end
    if (x - game.x) ^ 2 + (y - game.y) ^ 2 < 16 then x, y = -x, -y end
    local fast = game.elapsed >= 30 and math.random() < 0.3
    local radius = fast and 0.30 or 0.45
    game.enemies[#game.enemies + 1] = {
        entity = rectangle(fast and "FastEnemy" or "Enemy", x, y, radius * 2, radius * 2,
            fast and color.fast or color.normal, layer.enemy),
        x = x, y = y, radius = radius, hp = fast and 1 or 2, speed = fast and 3.1 or 1.75,
        tint = fast and color.fast or color.normal, flash = 0,
    }
end

local function update_spawning(dt)
    game.spawn_left = game.spawn_left - dt
    if game.spawn_left <= 0 then
        spawn_enemy()
        game.spawn_left = math.max(0.28, 0.9 - math.floor(game.elapsed / 30) * 0.12)
    end
end

local function update_shooting(dt)
    game.fire_left = math.max(0, game.fire_left - dt)
    local aim = game.aim_order[#game.aim_order]
    if not aim or game.fire_left > 0 or #game.bullets >= config.max_bullets then return end
    game.fire_left = config.fire_interval
    local x, y = game.x + aim.x * 0.55, game.y + aim.y * 0.55
    game.bullets[#game.bullets + 1] = {
        entity = rectangle("Bullet", x, y, aim.x ~= 0 and 0.45 or 0.18,
            aim.y ~= 0 and 0.45 or 0.18, color.bullet, layer.bullet),
        x = x, y = y, vx = aim.x * config.bullet_speed, vy = aim.y * config.bullet_speed,
        left = config.bullet_life,
    }
end

local function update_enemies(dt)
    for _, enemy in ipairs(game.enemies) do
        local dx, dy = normalized(game.x - enemy.x, game.y - enemy.y)
        local limit = config.half_size - enemy.radius
        enemy.x = clamp(enemy.x + dx * enemy.speed * dt, -limit, limit)
        enemy.y = clamp(enemy.y + dy * enemy.speed * dt, -limit, limit)
        enemy.entity:set_translation(enemy.x, enemy.y, 0)
        enemy.flash = math.max(0, enemy.flash - dt)
        enemy.entity.sprite.color = enemy.flash > 0 and color.white or enemy.tint
    end
end

-- Return the earliest intersection along a segment, including starting inside.
local function segment_hit(x, y, nx, ny, cx, cy, radius)
    local dx, dy, ox, oy = nx - x, ny - y, x - cx, y - cy
    local c = ox * ox + oy * oy - radius * radius
    if c <= 0 then return 0 end
    local a = dx * dx + dy * dy
    if a == 0 then return nil end
    local b = ox * dx + oy * dy
    local discriminant = b * b - a * c
    if discriminant < 0 then return nil end
    local t = (-b - math.sqrt(discriminant)) / a
    if t >= 0 and t <= 1 then return t end
    return nil
end

local function update_bullets(dt)
    for i = #game.bullets, 1, -1 do
        local bullet = game.bullets[i]
        local nx, ny = bullet.x + bullet.vx * dt, bullet.y + bullet.vy * dt
        local target, closest
        for _, enemy in ipairs(game.enemies) do
            if enemy.hp > 0 then
                local t = segment_hit(bullet.x, bullet.y, nx, ny, enemy.x, enemy.y,
                    enemy.radius + config.bullet_radius)
                if t and (not closest or t < closest) then target, closest = enemy, t end
            end
        end
        bullet.left = bullet.left - dt
        if target then
            target.hp, target.flash = target.hp - 1, 0.07
            target.entity.sprite.color = color.white
            burst(target.x, target.y, target.tint, target.hp == 0 and 8 or 3)
            if target.hp == 0 then game.kills = game.kills + 1; shake(0.045, 0.07) end
        end
        if target or bullet.left <= 0 or math.abs(nx) > config.half_size or math.abs(ny) > config.half_size then
            destroy(bullet.entity)
            table.remove(game.bullets, i)
        else
            bullet.x, bullet.y = nx, ny
            bullet.entity:set_translation(nx, ny, 0)
        end
    end
    -- Delete dead enemies only after every bullet has finished its traversal.
    for i = #game.enemies, 1, -1 do
        if game.enemies[i].hp <= 0 then
            destroy(game.enemies[i].entity)
            table.remove(game.enemies, i)
        end
    end
end

local function update_contacts()
    if game.dashing or game.hurt_left > 0 then return end
    for _, enemy in ipairs(game.enemies) do
        if (enemy.x - game.x) ^ 2 + (enemy.y - game.y) ^ 2 < (enemy.radius + config.player_radius) ^ 2 then
            game.health = game.health - 1
            game.hurt_left = config.hurt_time
            burst(game.x, game.y, color.normal, 12)
            shake(0.18, 0.18)
            if game.health == 0 then finish("lost") end
            return
        end
    end
end

local function simulate(dt)
    game.elapsed = math.min(config.duration, game.elapsed + dt)
    update_player(dt)
    update_spawning(dt)
    update_shooting(dt)
    update_enemies(dt)
    update_bullets(dt)
    update_contacts()
    if game.state == "playing" and game.elapsed >= config.duration - 1e-7 then
        game.elapsed = config.duration
        finish("won")
    end
end

local function update_effects(dt)
    for i = #game.particles, 1, -1 do
        local item = game.particles[i]
        item.left = item.left - dt
        if item.left <= 0 then
            destroy(item.entity)
            table.remove(game.particles, i)
        else
            item.x, item.y = item.x + item.vx * dt, item.y + item.vy * dt
            item.entity:set_translation(item.x, item.y, 0)
            local size = item.size * item.left / item.life
            item.entity:set_scale(size, size, 1)
        end
    end
    if game.state == "playing" then
        game.player.sprite.color = game.hurt_left > 0 and color.white or color.player
        game.player.sprite.visible = game.hurt_left <= 0 or math.floor(game.hurt_left * 16) % 2 == 0
    else
        game.player.sprite.visible = true
        game.player.sprite.color = game.state == "lost" and color.normal or color.player
    end
    game.shake_left = math.max(0, game.shake_left - dt)
    if camera then
        local strength = game.shake_strength * math.min(1, game.shake_left / 0.1)
        camera:set_translation(camera_base[1] + math.sin(Time.elapsed * 113) * strength,
            camera_base[2] + math.sin(Time.elapsed * 157) * strength, camera_base[3])
    end
    if game.shake_left == 0 then game.shake_strength = 0 end
end

function on_create()
    camera = scene:find("Camera")
    if camera and camera.valid then camera_base = { camera:get_translation() } else camera = nil end
    environment = scene.environment
    create_stage()
    reset("waiting")
    Log.info("Arena ready: Enter starts, WASD moves, arrows fire, Space dashes")
end

function on_update(dt)
    if dt ~= dt or dt < 0 or dt == math.huge then return end
    local frame_dt = math.min(dt, config.step * config.max_steps)
    if game.state ~= "playing" then
        if Input.is_key_pressed(Key.Enter) then reset("playing") end
    end
    if game.state == "playing" then
        read_input()
        game.accumulator = game.accumulator + frame_dt
        local steps = 0
        while game.accumulator + 1e-9 >= config.step and steps < config.max_steps and game.state == "playing" do
            game.accumulator = math.max(0, game.accumulator - config.step)
            simulate(config.step)
            steps = steps + 1
        end
    end
    update_effects(frame_dt)
    update_hud()
end

function on_destroy()
    clear_dynamic()
    for _, item in ipairs(owned) do destroy(item) end
    owned, hud, prompts = {}, {}, {}
    if camera and camera.valid then camera:set_translation(table.unpack(camera_base)) end
    if environment then environment.clear_color = color.background end
end
