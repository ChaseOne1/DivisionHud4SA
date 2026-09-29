#pragma once

#include "render/Gfx.hpp"
#include "render/TextRenderer.hpp"
#include "hud/Layout.hpp"

#include <d3d9.h>
#include <vector>

class CPed;

namespace division::hud {

// Health / armour / stamina / breath bars drawn above the panel card.
class Vitals
{
public:
    void Update(CPed* ped, float dt);
    void Draw(IDirect3DDevice9* device, render::PanelSurface const& surface, float panel_alpha,
        std::vector<render::BarVertex>& vertices) const;

private:
    float m_max_stamina = 0.0f;
    float m_max_air = 0.0f;
    float m_stamina_alpha = 0.0f;
    float m_stamina_hold = 0.0f;
    float m_breath_alpha = 0.0f;
    float m_breath_hold = 0.0f;
};

// Ammo numbers (magazine / reserve / previous / next weapon).
void DrawAmmoPanel(IDirect3DDevice9* device, render::TextRenderer& text, render::PanelSurface const& surface,
    AmmoZones const& zones, CPed* ped, float panel_alpha);

// Info panel icons (clock, wanted star) draw with the solid bar shader bound.
void DrawInfoIcons(IDirect3DDevice9* device, render::PanelSurface const& surface, PanelRects const& panels,
    float panel_alpha, std::vector<render::BarVertex>& vertices);

// Info panel text (money, clock, wanted count) draws with the text render states bound.
void DrawInfoText(IDirect3DDevice9* device, render::TextRenderer& text, render::PanelSurface const& surface,
    PanelRects const& panels, float panel_alpha);

}
