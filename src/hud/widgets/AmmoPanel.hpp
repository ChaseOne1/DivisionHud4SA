#pragma once

#include <d3d9.h>

#include "render/TextRenderer.hpp"
#include "hud/widgets/Layout.hpp"
#include "hud/widgets/PanelFrame.hpp"

class CPed;

namespace division::hud {

// Ammo numbers (magazine / reserve / previous / next weapon). Stateful so the
// magazine number can pop off the card on each shot and swap with the reserve
// number during a reload.
class AmmoPanel
{
public:
    void Update(CPed* ped, float dt);
    void Draw(IDirect3DDevice9* device, render::TextRenderer& text, PanelFrame const& frame,
        AmmoZones const& zones, CPed* ped, float panel_alpha) const;

private:
    bool m_firing_prev = false;
    float m_fire_kick = 0.0f;
    float m_reload_blend = 0.0f;
};

}