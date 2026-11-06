local speed = 3.0

function on_create()
    Log.info("Input mover ready: WASD moves, Space logs, Left mouse logs position")
end

function on_update(dt)
    local x, y, z = entity:get_translation()
    local move_x, move_z = 0, 0

    if Input.is_key_down(Key.A) then move_x = move_x - 1 end
    if Input.is_key_down(Key.D) then move_x = move_x + 1 end
    if Input.is_key_down(Key.W) then move_z = move_z - 1 end
    if Input.is_key_down(Key.S) then move_z = move_z + 1 end

    local length = math.sqrt(move_x * move_x + move_z * move_z)
    if length > 0 then
        x = x + move_x / length * speed * dt
        z = z + move_z / length * speed * dt
        entity:set_translation(x, y, z)
    end

    if Input.is_key_pressed(Key.Space) then
        Log.info("Space pressed by " .. entity.name)
    end

    if Input.is_mouse_button_pressed(MouseButton.Left) then
        Log.info(string.format("Mouse click at %.1f, %.1f", Input.mouse_x(), Input.mouse_y()))
    end
end
