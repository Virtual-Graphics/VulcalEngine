-- Tests inspect local state via debug upvalues; production exposes no test API.
local function upvalue(fn, expected)
    for i = 1, math.huge do
        local name, value = debug.getupvalue(fn, i)
        assert(name, "Missing upvalue: " .. expected)
        if name == expected then return value end
    end
end

local function state()
    return upvalue(on_update, "game")
end

local function near(actual, expected, tolerance)
    assert(math.abs(actual - expected) <= (tolerance or 1e-6),
        string.format("Expected %.9f, got %.9f", expected, actual))
end

local held, pressed = {}, {}
local function frame(dt, keys)
    keys = keys or {}
    pressed = {}
    for key in pairs(keys) do if not held[key] then pressed[key] = true end end
    held = keys
    Time.elapsed = Time.elapsed + dt
    on_update(dt)
end

local function steps(count, keys)
    for _ = 1, count do frame(1 / 120, keys) end
end

local function start()
    frame(0, { Enter = true })
    frame(0)
    assert(state().state == "playing")
end

local function quiet()
    start()
    state().spawn_left = math.huge
    return state()
end

local function count_entities()
    local count = 0
    for _ in pairs(scene.items) do count = count + 1 end
    return count
end

local function enemy(x, y, hp)
    local item = scene:create_entity("TestEnemy")
    item:add_sprite()
    local result = { entity = item, x = x, y = y, hp = hp or 2, speed = 0,
        radius = 0.45, tint = { 1, 0, 0, 1 }, flash = 0 }
    table.insert(state().enemies, result)
    return result
end

local function bullet(x, y, vx)
    local item = scene:create_entity("TestBullet")
    item:add_sprite()
    local result = { entity = item, x = x, y = y, vx = vx, vy = 0, left = 1 }
    table.insert(state().bullets, result)
    return result
end

function load_arena(path)
    math.randomseed(42)
    held, pressed = {}, {}
    Key = {}
    for _, name in ipairs({ "W", "A", "S", "D", "Up", "Down", "Left", "Right", "Space", "Enter" }) do
        Key[name] = name
    end
    Input = {
        is_key_down = function(key) return held[key] == true end,
        is_key_pressed = function(key) return pressed[key] == true end,
        key_axis = function(negative, positive) return (held[positive] and 1 or 0) - (held[negative] and 1 or 0) end,
    }
    Time = { elapsed = 0 }
    Log = { info = function() end }
    scene = { items = {}, environment = {} }
    function scene:create_entity(name)
        local item = { name = name, valid = true, x = 0, y = 0, z = 10 }
        function item:add_sprite()
            self.sprite = { visible = true }
            return self.sprite
        end
        function item:set_translation(x, y, z)
            assert(self.valid, "Moving a destroyed entity")
            self.x, self.y, self.z = x, y, z
        end
        function item:get_translation() return self.x, self.y, self.z end
        function item:set_scale(x, y, z)
            assert(self.valid, "Scaling a destroyed entity")
            self.sx, self.sy, self.sz = x, y, z
        end
        function item:destroy()
            assert(self.valid, "Double destruction")
            self.valid = false
            scene.items[self] = nil
        end
        self.items[item] = true
        return item
    end
    function scene:find(name)
        for item in pairs(self.items) do if item.name == name then return item end end
    end
    scene:create_entity("Camera")
    dofile(path)
    on_create()
end

arena_tests = {}

function arena_tests.waiting_and_cleanup()
    local count = count_entities()
    steps(120)
    assert(state().state == "waiting" and #state().enemies == 0)
    near(state().elapsed, 0)
    assert(count_entities() == count)
    on_destroy()
    assert(count_entities() == 1, "Only the scene-owned camera should remain")
    on_create()
    assert(count_entities() == count, "Starting the same script again must not duplicate the stage")
end

function arena_tests.movement_and_boundaries()
    local game = quiet()
    steps(60, { W = true, D = true })
    near(math.sqrt(game.x ^ 2 + game.y ^ 2), 3)
    steps(360, { W = true, D = true })
    near(game.x, 9.62)
    near(game.y, 9.62)
    local x, y = game.x, game.y
    steps(10, { W = true, S = true, A = true, D = true })
    near(game.x, x)
    near(game.y, y)
end

function arena_tests.frame_rate_and_bounded_catchup()
    local game = quiet()
    for _ = 1, 15 do frame(1 / 30, { D = true }) end
    local distance = game.x
    near(distance, 3)
    frame(0, { Enter = true }) -- Enter does not reset an active run.
    near(game.x, distance)
    game.x, game.y, game.elapsed, game.accumulator = 0, 0, 0, 0
    for _ = 1, 72 do frame(1 / 144, { D = true }) end
    near(game.x, distance)
    local before = game.elapsed
    frame(10, { D = true })
    near(game.elapsed - before, 0.1)
    local x = game.x
    frame(-1, { D = true })
    frame(0 / 0, { D = true })
    frame(math.huge, { D = true })
    near(game.x, x)
end

function arena_tests.aim_priority_and_fire_cadence()
    local game = quiet()
    steps(1, { Right = true })
    assert(#game.bullets == 1 and game.bullets[1].vx > 0)
    steps(1, { Right = true, Up = true })
    assert(game.aim_order[#game.aim_order].key == Key.Up)
    steps(17, { Right = true, Up = true })
    assert(#game.bullets == 2 and game.bullets[2].vy > 0)
    steps(1, { Right = true })
    assert(game.aim_order[#game.aim_order].key == Key.Right)
    steps(17)
    assert(#game.bullets == 2, "Releasing all arrows must stop firing")
end

function arena_tests.substep_input_is_not_lost_or_repeated()
    local game = quiet()
    frame(1 / 1000, { Space = true, Right = true })
    assert(game.dash_requested and #game.bullets == 0)
    frame(1 / 1000)
    frame(1 / 120)
    assert(game.dash_cooldown > 0 and #game.bullets == 0)
    steps(30, { Space = true })
    assert(game.dash_left == 0, "Holding Space must not repeatedly dash")
end

function arena_tests.dash_duration_cooldown_and_bounds()
    local game = quiet()
    steps(18, { Space = true })
    near(game.y, 3)
    steps(30, { Space = true })
    near(game.y, 3)
    frame(0)
    steps(1, { Space = true })
    near(game.y, 3)
    steps(90)
    steps(18, { Space = true })
    near(game.y, 6)
    game.x, game.y, game.dash_cooldown = 9.5, 0, 0
    frame(0)
    steps(18, { Space = true, D = true })
    near(game.x, 9.62)
end

function arena_tests.hurt_invulnerability_and_loss()
    local game = quiet()
    enemy(0, 0)
    steps(1)
    assert(game.health == 2)
    steps(100)
    assert(game.health == 2, "Contact must not damage every simulation step")
    steps(30)
    assert(game.health == 1)
    steps(130)
    assert(game.health == 0 and game.state == "lost")
    local elapsed = game.elapsed
    steps(120)
    near(game.elapsed, elapsed)
    assert(#game.particles == 0 and game.player.sprite.visible)
    near(scene:find("Camera").x, 0)
    near(scene:find("Camera").y, 0)
end

function arena_tests.dash_invulnerability()
    local game = quiet()
    enemy(0, 0.3)
    steps(1, { Space = true })
    assert(game.health == 3 and game.dashing)
end

function arena_tests.swept_hits_nearest_enemy_and_kills_once()
    local game = quiet()
    local far = enemy(3, 2, 1)
    local close = enemy(-2, 2, 2)
    local shot = bullet(-5, 2, 1200)
    steps(1)
    assert(close.hp == 1 and far.hp == 1 and close.flash > 0)
    assert(not shot.entity.valid and #game.bullets == 0)
    bullet(-5, 2, 1200)
    bullet(-5, 2, 1200)
    steps(1)
    assert(game.kills == 2 and #game.enemies == 0)
    assert(not close.entity.valid and not far.entity.valid)
    assert(#game.bullets == 0)
end

function arena_tests.bullet_expiry_and_misses()
    local game = quiet()
    local target = enemy(0, 4, 2)
    bullet(-5, 2, 1200)
    steps(1)
    assert(target.hp == 2)
    steps(2)
    assert(#game.bullets == 0)
    local shot = bullet(0, 2, 0)
    shot.left = 1 / 240
    steps(1)
    assert(not shot.entity.valid and #game.bullets == 0)
end

function arena_tests.spawning_difficulty_and_caps()
    start()
    local game = state()
    game.hurt_left = 1000
    steps(120)
    assert(#game.enemies == 1 and game.enemies[1].hp == 2)
    game.spawn_left = 0
    steps(1)
    local first_interval = game.spawn_left
    game.elapsed, game.spawn_left = 31, 0
    game.x, game.y = 9.4, 0
    local saw_fast = false
    for _ = 1, 150 do
        game.spawn_left = 0
        steps(1)
        for _, item in ipairs(game.enemies) do
            if item.entity.name == "FastEnemy" then saw_fast = true end
        end
        local item = game.enemies[#game.enemies]
        assert((item.x - game.x) ^ 2 + (item.y - game.y) ^ 2 > 15,
            "New enemies must not spawn next to the player")
    end
    assert(#game.enemies == 90 and saw_fast)
    assert(game.spawn_left < first_interval)
    for _ = 1, 48 do bullet(0, 0, 0) end
    steps(1, { Right = true })
    assert(#game.bullets == 48, "Firing at capacity must not add another bullet")
end

function arena_tests.victory_and_damage_precedence()
    local game = quiet()
    steps(180 * 120)
    assert(game.state == "won")
    near(game.elapsed, 180)
    steps(120)
    near(game.elapsed, 180)
    frame(0, { Enter = true })
    game = state()
    game.spawn_left, game.elapsed, game.health = math.huge, 180 - 1 / 120, 1
    enemy(0, 0)
    steps(1)
    assert(game.state == "lost", "Fatal contact on the final step takes precedence over victory")
end

function arena_tests.restarts_clear_dynamic_entities_and_effects()
    local baseline = count_entities()
    for _ = 1, 5 do
        start()
        local game = state()
        game.health, game.spawn_left = 1, math.huge
        enemy(0, 0)
        bullet(0, 3, 0)
        steps(1)
        assert(game.state == "lost" and #game.particles > 0)
        frame(0, { Enter = true })
        game = state()
        assert(game.state == "playing" and game.health == 3 and game.kills == 0)
        assert(#game.enemies == 0 and #game.bullets == 0 and #game.particles == 0)
        near(game.elapsed, 0)
        near(game.accumulator, 0)
        near(scene:find("Camera").x, 0)
        near(scene:find("Camera").y, 0)
        assert(count_entities() == baseline, "Restart leaked entities")
        -- Return to a terminal state so the next loop can start through Enter.
        game.health = 1
        enemy(0, 0)
        steps(1)
        frame(0)
    end
end

function arena_tests.particle_budget_and_terminal_freeze()
    local game = quiet()
    for i = 1, 40 do
        enemy(1, 3, 1)
        bullet(0, 3, 240)
    end
    steps(1)
    assert(game.kills == 40 and #game.particles <= 160)
    game.health = 1
    local target = enemy(0, 0)
    steps(1)
    local x, elapsed = target.x, game.elapsed
    steps(120, { W = true, Right = true, Space = true })
    near(target.x, x)
    near(game.elapsed, elapsed)
    assert(#game.particles == 0 and #game.bullets == 0)
end
