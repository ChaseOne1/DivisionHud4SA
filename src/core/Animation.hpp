#pragma once

#include <algorithm>

#include "CTimer.h"

namespace division::core {
constexpr float TIME_STEP_TO_SECONDS = 0.02f;
constexpr float MIN_FADE_DURATION = 0.02f;
constexpr float MAX_FADE_STEP = 0.5f;

// Seconds elapsed since the last game frame; falls back to a nominal 60 fps value
// when the timer is not available this frame.
inline float FrameDeltaSeconds(float fallback = 1.0f / 60.0f)
{
    float const dt = CTimer::ms_fTimeStep * TIME_STEP_TO_SECONDS;
    return (dt > 0.0f) ? dt : fallback;
}

// Moves `alpha` toward `target` at a rate of (1/fade) per second, split into a
// faster close-in by capping the per-advance step. Fade-out never lingers below 1 px.
inline float AdvanceFade(float alpha, float target, float dt, float fade_in, float fade_out)
{
    float const fade = (alpha < target) ? std::max(MIN_FADE_DURATION, fade_in) : std::max(MIN_FADE_DURATION, fade_out);
    float const step = std::min(MAX_FADE_STEP, dt / fade);
    if (alpha < target) return std::min(1.0f, alpha + step);
    if (alpha > target) return std::max(0.0f, alpha - step);
    return alpha;
}
}