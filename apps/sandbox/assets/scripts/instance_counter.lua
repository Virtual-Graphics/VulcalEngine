local updates = 0
local elapsed = 0

function on_create()
    Log.info("Independent script instance started for " .. entity.name)
end

function on_update(dt)
    updates = updates + 1
    elapsed = elapsed + dt

    if updates % 120 == 0 then
        Log.info(string.format("%s: %d updates, %.1f seconds", entity.name, updates, elapsed))
    end
end

function on_destroy()
    Log.info(string.format("%s finished after %d updates", entity.name, updates))
end
