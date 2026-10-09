#pragma once

#include "render/Surface.hpp"

namespace division::hud {

// Screen-space frame of the HUD card. It wraps the same on-screen quad the card
// body uses (the world <-> aim-down-sights blend), so anything mapped through it
// tracks the card in every camera mode. `pop` pushes a point outward from the card
// centre, which reads as the number lifting off the card toward the camera.
struct PanelFrame
{
    render::PanelSurface surface { };

    render::ScreenPoint Map(float nx, float ny, float pop) const
    {
        render::ScreenPoint const point = surface.Map(nx, ny);
        if (pop == 0.0f) return point;

        render::ScreenPoint const centre = surface.Map(0.5f, 0.5f);
        float const scale = 1.0f + pop;
        return render::ScreenPoint { centre.x + (point.x - centre.x) * scale,
            centre.y + (point.y - centre.y) * scale };
    }
};

}