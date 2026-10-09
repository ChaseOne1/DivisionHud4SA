#pragma once

#include <d3d9.h>
#include <vector>

#include "render/Surface.hpp"

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

    // Per-bar animated presence (0..1): drives stack reflow, bar height and fade.
    enum { BAR_HEALTH, BAR_ARMOR, BAR_STAMINA, BAR_BREATH, BAR_COUNT };
    float m_bar_presence[BAR_COUNT] = { 1.0f, 0.0f, 0.0f, 0.0f };

    bool m_health_initialised = false;
    float m_health_prev = 0.0f;
    float m_health_chip = 0.0f;
    float m_health_chip_hold = 0.0f;
    float m_health_pulse = 0.0f;
    float m_health_pulse_hold = 0.0f;
};

}