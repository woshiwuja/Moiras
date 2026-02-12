#pragma once

namespace moiras {

// Simple 2D vector to replace Raylib's Vector2
struct Vec2 {
    float x;
    float y;

    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x, float y) : x(x), y(y) {}
};

} // namespace moiras
