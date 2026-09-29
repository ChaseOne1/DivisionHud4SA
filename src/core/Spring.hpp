#pragma once

#include <cmath>
#include "CVector.h"

namespace division::core {
inline void Damp(CVector& value, CVector& velocity, CVector const& target, float omega, float dt)
{
    if (dt <= 0.0f)
        return;
    if (dt > 0.1f)
        dt = 0.1f;

    float const decay = std::exp(-omega * dt);
    CVector const offset = value - target;
    CVector const step = (velocity + offset * omega) * dt;

    value = target + (offset + step) * decay;
    velocity = (velocity - step * omega) * decay;
}
}
