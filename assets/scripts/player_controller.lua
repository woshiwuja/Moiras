-- assets/scripts/player_controller.lua
-- Example script demonstrating the Moiras Lua scripting system

local speed = 5.0
local health = 100
local time_alive = 0

function on_start()
    print("Player script started on: " .. self.name)
    print("Position: " .. tostring(self.position))
end

function on_update(dt)
    time_alive = time_alive + dt

    -- WASD movement
    local move = vec3(0, 0, 0)

    if Input.is_key_down(Input.KEY_W) then
        move.z = move.z - 1
    end
    if Input.is_key_down(Input.KEY_S) then
        move.z = move.z + 1
    end
    if Input.is_key_down(Input.KEY_A) then
        move.x = move.x - 1
    end
    if Input.is_key_down(Input.KEY_D) then
        move.x = move.x + 1
    end

    if move:length() > 0 then
        move = move:normalized() * speed * dt
        self.position = self.position + move
    end

    -- Example: find another object by name
    local enemy = World.find_by_name("Enemy")
    if enemy then
        local dist = self.position:distance(enemy.position)
        if dist < 5.0 then
            print("Enemy nearby! Distance: " .. tostring(dist))
        end
    end
end

function on_destroy()
    print("Player script destroyed! Was alive for " .. tostring(time_alive) .. " seconds")
end

-- Custom function callable from other scripts or C++
function take_damage(amount)
    health = health - amount
    print(self.name .. " took " .. tostring(amount) .. " damage! Health: " .. tostring(health))
    if health <= 0 then
        print(self.name .. " has been defeated!")
    end
end
