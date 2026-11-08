local elapsed = 0
local start_x, start_y, start_z = 0, 0, 0

function on_create()
    start_x, start_y, start_z = entity:get_translation()
    Log.info("Transform demo started for " .. entity.name)
end

function on_update(dt)
    elapsed = elapsed + dt

    entity:set_rotation(0, elapsed, 0)
    entity:set_translation(start_x, start_y + math.sin(elapsed * 2) * 0.5, start_z)
end

function on_destroy()
    Log.info("Transform demo stopped for " .. entity.name)
end
