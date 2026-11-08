local GRID_W = 20
local GRID_H = 20
local BASE_STEP = 0.16
local MIN_STEP = 0.07
local SCORE_STEP_BONUS = 0.004

local CLEAR_COLOR = { 0.05, 0.06, 0.08, 1.0 }
local GAME_OVER_CLEAR = { 0.22, 0.04, 0.04, 1.0 }
local WIN_CLEAR = { 0.08, 0.20, 0.10, 1.0 }
local BOARD_COLOR = { 0.10, 0.14, 0.12, 1.0 }
local WALL_COLOR = { 0.30, 0.36, 0.32, 1.0 }
local HEAD_COLOR = { 0.50, 1.00, 0.50, 1.0 }
local BODY_COLOR = { 0.20, 0.72, 0.30, 1.0 }
local FOOD_COLOR = { 1.00, 0.30, 0.18, 1.0 }
local DEAD_COLOR = { 0.90, 0.15, 0.15, 1.0 }
local WIN_COLOR = { 1.00, 0.85, 0.25, 1.0 }
local EAT_FLASH = { 0.12, 0.30, 0.18, 1.0 }
local DEATH_FLASH = { 1.00, 0.10, 0.08, 1.0 }
local WIN_FLASH = { 1.00, 0.90, 0.35, 1.0 }

local SORT_BOARD = -100
local SORT_WALL = -50
local SORT_BODY = 10
local SORT_HEAD = 11
local SORT_FOOD = 20
local SORT_PARTICLE = 40

local MAX_PARTICLES = 240

local snake = {}
local food = nil
local direction = { x = 1, y = 0 }
local next_direction = { x = 1, y = 0 }
local direction_locked = false
local move_timer = 0.0
local score = 0
local step_seconds = BASE_STEP
local game_over = false
local won = false

local camera_entity = nil
local camera_proxy = nil
local camera_base = { x = 0.0, y = 0.0, z = 10.0 }
local camera_top = 12.0
local camera_bottom = -12.0
local shake_time = 0.0
local shake_duration = 0.001
local shake_strength = 0.0
local shake_seed = 0.0
local zoom_pulse = 0.0
local flash_time = 0.0
local flash_duration = 0.001
local flash_color = { 0.0, 0.0, 0.0, 1.0 }
local background_color = CLEAR_COLOR
local head_pulse = 0.0
local particles = {}

local function copy_color(color)
    return { color[1], color[2], color[3], color[4] or 1.0 }
end

local function set_clear_color(color)
    local environment = scene.environment
    if environment then environment.clear_color = color end
end

local function update_clear_color(dt)
    local color = background_color
    if flash_time > 0.0 then
        flash_time = math.max(0.0, flash_time - dt)
        local t = flash_time / flash_duration
        color = {
            background_color[1] + (flash_color[1] - background_color[1]) * t,
            background_color[2] + (flash_color[2] - background_color[2]) * t,
            background_color[3] + (flash_color[3] - background_color[3]) * t,
            1.0,
        }
    end
    set_clear_color(color)
end

local function cell_to_world(gx, gy)
    return gx - (GRID_W - 1) * 0.5, gy - (GRID_H - 1) * 0.5, 0.0
end

local function make_block(name, gx, gy, color, sorting_order, scale)
    local block = scene:create_entity(name)
    local sprite = block:add_sprite()
    sprite.color = color
    sprite.sorting_order = sorting_order
    block:set_scale(scale, scale, 1.0)
    local x, y, z = cell_to_world(gx, gy)
    block:set_translation(x, y, z)
    return block
end

local function make_rect(name, x, y, sx, sy, color, sorting_order)
    local block = scene:create_entity(name)
    local sprite = block:add_sprite()
    sprite.color = color
    sprite.sorting_order = sorting_order
    block:set_scale(sx, sy, 1.0)
    block:set_translation(x, y, 0.0)
    return block
end

local function create_board()
    local half_w = GRID_W * 0.5
    local half_h = GRID_H * 0.5
    make_rect("Board", 0, 0, GRID_W, GRID_H, BOARD_COLOR, SORT_BOARD)
    make_rect("WallTop", 0, half_h, GRID_W + 0.4, 0.4, WALL_COLOR, SORT_WALL)
    make_rect("WallBottom", 0, -half_h, GRID_W + 0.4, 0.4, WALL_COLOR, SORT_WALL)
    make_rect("WallLeft", -half_w, 0, 0.4, GRID_H, WALL_COLOR, SORT_WALL)
    make_rect("WallRight", half_w, 0, 0.4, GRID_H, WALL_COLOR, SORT_WALL)
end

local function create_snake()
    local cx = math.floor(GRID_W / 2)
    local cy = math.floor(GRID_H / 2)
    snake = {}
    snake[1] = { x = cx, y = cy, entity = make_block("SnakeHead", cx, cy, HEAD_COLOR, SORT_HEAD, 0.95) }
    snake[2] = { x = cx - 1, y = cy, entity = make_block("SnakeBody", cx - 1, cy, BODY_COLOR, SORT_BODY, 0.90) }
    snake[3] = { x = cx - 2, y = cy, entity = make_block("SnakeBody", cx - 2, cy, BODY_COLOR, SORT_BODY, 0.90) }
end

local function trigger_flash(color, duration)
    flash_color = copy_color(color)
    flash_duration = math.max(duration, 0.001)
    flash_time = flash_duration
end

local function trigger_shake(duration, strength)
    shake_time = math.max(shake_time, duration)
    shake_duration = math.max(duration, 0.001)
    shake_strength = math.max(shake_strength, strength)
end

local function trigger_zoom(amount)
    zoom_pulse = math.max(zoom_pulse, amount)
end

local function reset_camera()
    if camera_entity and camera_entity.valid then
        camera_entity:set_translation(camera_base.x, camera_base.y, camera_base.z)
        camera_entity:set_rotation(0, 0, 0)
    end
    if camera_proxy then
        camera_proxy.top = camera_top
        camera_proxy.bottom = camera_bottom
    end
    shake_time = 0.0
    shake_strength = 0.0
    zoom_pulse = 0.0
end

local function update_camera_effects(dt)
    if not camera_entity or not camera_entity.valid then return end

    if shake_time > 0.0 then
        shake_time = math.max(0.0, shake_time - dt)
        shake_strength = shake_strength * math.exp(-3.0 * dt)
    else
        shake_strength = 0.0
    end

    local amplitude = 0.0
    if shake_time > 0.0 then amplitude = shake_strength * (shake_time / shake_duration) end

    local offset_x = math.sin((Time.elapsed + shake_seed) * 70.0) * amplitude
    local offset_y = math.cos((Time.elapsed + shake_seed) * 83.0) * amplitude * 0.8
    local roll = math.sin((Time.elapsed + shake_seed) * 61.0) * amplitude * 0.06

    camera_entity:set_translation(camera_base.x + offset_x, camera_base.y + offset_y, camera_base.z)
    camera_entity:set_rotation(0, 0, roll)

    if zoom_pulse > 0.0 then
        zoom_pulse = math.max(0.0, zoom_pulse - dt * 4.0)
    else
        zoom_pulse = math.min(0.0, zoom_pulse + dt * 2.5)
    end
    if camera_proxy then
        camera_proxy.top = camera_top - zoom_pulse
        camera_proxy.bottom = camera_bottom + zoom_pulse
    end
end

local function spawn_particles_at_world(x, y, color, count, speed, size)
    if #particles >= MAX_PARTICLES then return end
    count = math.min(count, MAX_PARTICLES - #particles)

    for _ = 1, count do
        local angle = math.random() * math.pi * 2.0
        local velocity = speed * (0.4 + math.random() * 0.9)
        local particle = {
            x = x,
            y = y,
            vx = math.cos(angle) * velocity,
            vy = math.sin(angle) * velocity,
            life = 0.35 + math.random() * 0.30,
            max_life = 0.0,
            size = size * (0.7 + math.random() * 0.8),
            color = copy_color(color),
            entity = nil,
        }
        particle.max_life = particle.life

        local entity = scene:create_entity("Particle")
        local sprite = entity:add_sprite()
        sprite.color = particle.color
        sprite.sorting_order = SORT_PARTICLE
        entity:set_scale(particle.size, particle.size, 1.0)
        entity:set_translation(particle.x, particle.y, 0.0)
        particle.entity = entity

        particles[#particles + 1] = particle
    end
end

local function spawn_particles_at_cell(gx, gy, color, count, speed, size)
    local x, y = cell_to_world(gx, gy)
    spawn_particles_at_world(x, y, color, count, speed, size)
end

local function clear_particles()
    for i = #particles, 1, -1 do
        local particle = particles[i]
        if particle.entity and particle.entity.valid then particle.entity:destroy() end
        particles[i] = nil
    end
end

local function update_particles(dt)
    for i = #particles, 1, -1 do
        local particle = particles[i]
        particle.life = particle.life - dt
        if particle.life <= 0.0 or not particle.entity or not particle.entity.valid then
            if particle.entity and particle.entity.valid then particle.entity:destroy() end
            table.remove(particles, i)
        else
            particle.x = particle.x + particle.vx * dt
            particle.y = particle.y + particle.vy * dt
            local damping = math.exp(-2.5 * dt)
            particle.vx = particle.vx * damping
            particle.vy = particle.vy * damping - 1.8 * dt

            local t = particle.life / particle.max_life
            local scale = particle.size * (0.35 + 0.65 * t)
            particle.entity:set_translation(particle.x, particle.y, 0.0)
            particle.entity:set_scale(scale, scale, 1.0)
            local sprite = particle.entity.sprite
            if sprite then
                sprite.color = { particle.color[1], particle.color[2], particle.color[3], t }
            end
        end
    end
end

local function end_game(is_win)
    if game_over then return end
    game_over = true
    won = is_win

    if food and food.entity and food.entity.valid then food.entity:destroy() end
    food = nil

    if is_win then
        background_color = WIN_CLEAR
        trigger_flash(WIN_FLASH, 0.45)
        trigger_shake(0.50, 0.22)
        zoom_pulse = 0.35
        Log.info(string.format("You win! Final score: %d", score))
        for _, segment in ipairs(snake) do
            local sprite = segment.entity.sprite
            if sprite then sprite.color = WIN_COLOR end
            spawn_particles_at_cell(segment.x, segment.y, WIN_COLOR, 2, 2.2, 0.16)
        end
    else
        background_color = GAME_OVER_CLEAR
        trigger_flash(DEATH_FLASH, 0.35)
        trigger_shake(0.55, 0.35)
        zoom_pulse = -0.8
        Log.info(string.format("Game Over. Final score: %d", score))
        for _, segment in ipairs(snake) do
            local sprite = segment.entity.sprite
            if sprite then sprite.color = DEAD_COLOR end
            spawn_particles_at_cell(segment.x, segment.y, DEAD_COLOR, 3, 2.8, 0.17)
        end
    end
end

local function spawn_food()
    local occupied = {}
    for _, segment in ipairs(snake) do
        occupied[segment.x .. ":" .. segment.y] = true
    end

    local free = {}
    for y = 0, GRID_H - 1 do
        for x = 0, GRID_W - 1 do
            if not occupied[x .. ":" .. y] then
                free[#free + 1] = { x = x, y = y }
            end
        end
    end

    if #free == 0 then
        end_game(true)
        return
    end

    if food and food.entity and food.entity.valid then food.entity:destroy() end
    local cell = free[math.random(#free)]
    food = { x = cell.x, y = cell.y, entity = make_block("Food", cell.x, cell.y, FOOD_COLOR, SORT_FOOD, 0.75) }
    spawn_particles_at_cell(cell.x, cell.y, FOOD_COLOR, 6, 1.6, 0.12)
end

local function reset_game()
    for _, segment in ipairs(snake) do
        if segment.entity and segment.entity.valid then segment.entity:destroy() end
    end
    snake = {}
    if food and food.entity and food.entity.valid then food.entity:destroy() end
    food = nil
    clear_particles()
    reset_camera()

    direction = { x = 1, y = 0 }
    next_direction = { x = 1, y = 0 }
    direction_locked = false
    move_timer = 0.0
    score = 0
    step_seconds = BASE_STEP
    game_over = false
    won = false
    head_pulse = 0.0
    flash_time = 0.0
    background_color = CLEAR_COLOR
    set_clear_color(CLEAR_COLOR)

    create_snake()
    spawn_food()
end

local function step()
    if game_over then return end

    direction = next_direction
    direction_locked = false

    local head = snake[1]
    local new_head = { x = head.x + direction.x, y = head.y + direction.y }
    if new_head.x < 0 or new_head.x >= GRID_W or new_head.y < 0 or new_head.y >= GRID_H then
        end_game(false)
        return
    end

    local eating = food ~= nil and new_head.x == food.x and new_head.y == food.y
    local limit = eating and #snake or (#snake - 1)
    for i = 1, limit do
        if snake[i].x == new_head.x and snake[i].y == new_head.y then
            end_game(false)
            return
        end
    end

    local old_head_x = head.x
    local old_head_y = head.y
    local tail_x = snake[#snake].x
    local tail_y = snake[#snake].y
    for i = #snake, 2, -1 do
        snake[i].x = snake[i - 1].x
        snake[i].y = snake[i - 1].y
    end
    snake[1].x = new_head.x
    snake[1].y = new_head.y
    spawn_particles_at_cell(old_head_x, old_head_y, BODY_COLOR, 2, 0.7, 0.10)

    if eating then
        if food and food.entity and food.entity.valid then food.entity:destroy() end
        food = nil

        snake[#snake + 1] = {
            x = tail_x,
            y = tail_y,
            entity = make_block("SnakeBody", tail_x, tail_y, BODY_COLOR, SORT_BODY, 0.90),
        }
        score = score + 1
        step_seconds = math.max(MIN_STEP, BASE_STEP - score * SCORE_STEP_BONUS)
        head_pulse = 1.0
        trigger_flash(EAT_FLASH, 0.16)
        trigger_shake(0.22, 0.12)
        trigger_zoom(0.22)
        spawn_particles_at_cell(new_head.x, new_head.y, FOOD_COLOR, 14, 3.2, 0.18)
        Log.info(string.format("Score: %d", score))
        spawn_food()
    end

    for _, segment in ipairs(snake) do
        local x, y, z = cell_to_world(segment.x, segment.y)
        segment.entity:set_translation(x, y, z)
    end
end

local function set_direction(x, y)
    if game_over or direction_locked then return end
    if direction.x == -x and direction.y == -y then return end
    next_direction = { x = x, y = y }
    direction_locked = true
end

local function update_input()
    if game_over then
        if Input.is_key_pressed(Key.Space) then reset_game() end
        return
    end
    if Input.is_key_pressed(Key.W) or Input.is_key_pressed(Key.Up) then set_direction(0, 1) end
    if Input.is_key_pressed(Key.S) or Input.is_key_pressed(Key.Down) then set_direction(0, -1) end
    if Input.is_key_pressed(Key.A) or Input.is_key_pressed(Key.Left) then set_direction(-1, 0) end
    if Input.is_key_pressed(Key.D) or Input.is_key_pressed(Key.Right) then set_direction(1, 0) end
end

local function update_effects(dt)
    update_clear_color(dt)
    update_camera_effects(dt)
    update_particles(dt)

    if food and food.entity and food.entity.valid then
        local pulse = 0.72 + (math.sin(Time.elapsed * 10.0) + 1.0) * 0.05
        food.entity:set_scale(pulse, pulse, 1.0)
    end

    head_pulse = math.max(0.0, head_pulse - dt * 4.0)
    if snake[1] and snake[1].entity and snake[1].entity.valid then
        local scale = 0.95 + head_pulse * 0.25
        snake[1].entity:set_scale(scale, scale, 1.0)
    end
end

function on_create()
    math.randomseed(20260101)
    create_board()

    camera_entity = scene:find("Camera")
    if camera_entity and camera_entity.valid then
        local x, y, z = camera_entity:get_translation()
        camera_base = { x = x, y = y, z = z }
        camera_proxy = camera_entity.camera
        if camera_proxy then
            camera_top = camera_proxy.top
            camera_bottom = camera_proxy.bottom
        end
    end
    shake_seed = math.random() * 1000.0

    reset_game()
    Log.info("Snake ready: WASD/Arrows to move, Space to restart")
end

function on_update(dt)
    update_input()
    update_effects(dt)

    if game_over then return end

    move_timer = move_timer + dt
    local steps = 0
    while move_timer >= step_seconds and not game_over and steps < 5 do
        move_timer = move_timer - step_seconds
        step()
        steps = steps + 1
    end
end

function on_destroy()
    Log.info(string.format("Snake stopped. Final score: %d", score))
end
