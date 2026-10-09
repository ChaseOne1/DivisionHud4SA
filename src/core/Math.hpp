#pragma once

namespace division::core {
// Single-precision constants shared by every drawing file (Gfx/Panel3D/DamageIndicators).
inline constexpr float PI_F = 3.1415926535897932f;
inline constexpr float DEG_TO_RAD = PI_F / 180.0f;

// Normalises an angle into [-PI, PI).
inline float WrapAngle(float angle)
{
    while (angle > PI_F)
        angle -= 2.0f * PI_F;
    while (angle < -PI_F)
        angle += 2.0f * PI_F;
    return angle;
}
}