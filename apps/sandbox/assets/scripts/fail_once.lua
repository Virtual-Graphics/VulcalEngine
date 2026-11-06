local failed = false

function on_create()
    Log.warn("Failure isolation demo attached to " .. entity.name)
end

function on_update(dt)
    if not failed then
        failed = true
        error("Intentional test error: this instance should stop updating")
    end
end

function on_destroy()
    Log.info("Failure isolation demo destroyed for " .. entity.name)
end
