#include "MathBindings.hpp"
#include <raylib.h>
#include <raymath.h>
#include <string>

namespace moiras
{

  void MathBindings::registerBindings(sol::state &lua)
  {
    // Register Vector3 as a usertype named "Vector3" internally
    lua.new_usertype<Vector3>("Vector3",
                              sol::call_constructor, sol::factories(
                                                         []() -> Vector3
                                                         { return {0.0f, 0.0f, 0.0f}; },
                                                         [](float x, float y, float z) -> Vector3
                                                         { return {x, y, z}; }),

                              "x", &Vector3::x,
                              "y", &Vector3::y,
                              "z", &Vector3::z,

                              sol::meta_function::addition, [](const Vector3 &a, const Vector3 &b)
                              { return Vector3Add(a, b); },
                              sol::meta_function::subtraction, [](const Vector3 &a, const Vector3 &b)
                              { return Vector3Subtract(a, b); },
                              sol::meta_function::multiplication, sol::overload(
                                                                      [](const Vector3 &v, float s)
                                                                      { return Vector3Scale(v, s); },
                                                                      [](float s, const Vector3 &v)
                                                                      { return Vector3Scale(v, s); }),
                              sol::meta_function::unary_minus, [](const Vector3 &v)
                              { return Vector3Negate(v); },
                              sol::meta_function::equal_to, [](const Vector3 &a, const Vector3 &b)
                              { return (a.x == b.x && a.y == b.y && a.z == b.z); },
                              sol::meta_function::to_string, [](const Vector3 &v)
                              { return "vec3(" + std::to_string(v.x) + ", " +
                                       std::to_string(v.y) + ", " + std::to_string(v.z) + ")"; },

                              "length", [](const Vector3 &v)
                              { return Vector3Length(v); },
                              "normalized", [](const Vector3 &v)
                              { return Vector3Normalize(v); },
                              "dot", [](const Vector3 &a, const Vector3 &b)
                              { return Vector3DotProduct(a, b); },
                              "cross", [](const Vector3 &a, const Vector3 &b)
                              { return Vector3CrossProduct(a, b); },
                              "distance", [](const Vector3 &a, const Vector3 &b)
                              { return Vector3Distance(a, b); },
                              "lerp", [](const Vector3 &a, const Vector3 &b, float t)
                              { return Vector3Lerp(a, b, t); });

    // Convenience global constructor: vec3(x, y, z)
    lua.set_function("vec3", sol::overload(
                                 []() -> Vector3
                                 { return {0.0f, 0.0f, 0.0f}; },
                                 [](float x, float y, float z) -> Vector3
                                 { return {x, y, z}; }));

    // Quaternion
    lua.new_usertype<Quaternion>("Quaternion",
                                 sol::call_constructor, sol::factories(
                                                            []() -> Quaternion
                                                            { return QuaternionIdentity(); },
                                                            [](float x, float y, float z, float w) -> Quaternion
                                                            { return {x, y, z, w}; }),

                                 "x", &Quaternion::x,
                                 "y", &Quaternion::y,
                                 "z", &Quaternion::z,
                                 "w", &Quaternion::w,

                                 "to_euler", [](const Quaternion &q)
                                 { return QuaternionToEuler(q); },
                                 "normalize", [](const Quaternion &q)
                                 { return QuaternionNormalize(q); },
                                 "identity", []()
                                 { return QuaternionIdentity(); });

    // from_euler as a standalone function
    lua.set_function("quat_from_euler", [](float pitch, float yaw, float roll) -> Quaternion
                     { return QuaternionFromEuler(pitch, yaw, roll); });

    // Color
    lua.new_usertype<Color>("Color",
                            sol::call_constructor, sol::factories(
                                                       [](unsigned char r, unsigned char g, unsigned char b, unsigned char a) -> Color
                                                       { return {r, g, b, a}; },
                                                       [](unsigned char r, unsigned char g, unsigned char b) -> Color
                                                       { return {r, g, b, 255}; }),
                            "r", &Color::r,
                            "g", &Color::g,
                            "b", &Color::b,
                            "a", &Color::a);

    // Predefined colors
    lua["RED"] = Color{230, 41, 55, 255};
    lua["GREEN"] = Color{0, 228, 48, 255};
    lua["BLUE"] = Color{0, 121, 241, 255};
    lua["WHITE"] = Color{255, 255, 255, 255};
    lua["BLACK"] = Color{0, 0, 0, 255};
    lua["YELLOW"] = Color{253, 249, 0, 255};
    lua["ORANGE"] = Color{255, 161, 0, 255};
    lua["PURPLE"] = Color{200, 122, 255, 255};
    lua["GRAY"] = Color{130, 130, 130, 255};
  }

} // namespace moiras
