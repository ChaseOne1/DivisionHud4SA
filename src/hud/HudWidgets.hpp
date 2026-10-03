#pragma once

#include "render/Gfx.hpp"
#include "render/TextRenderer.hpp"
#include "hud/Layout.hpp"
#include "hud/PanelFrame.hpp"

#include <d3d9.h>
#include <vector>

class CPed;
class CWanted;

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

// Wanted-tier progress overlay: a left-to-right fill for the chaos progress inside
// the current wanted tier, plus a highlight that periodically sweeps the fill. Only
// active while at least one wanted star is present. Draws under the info icons/text.
class WantedBar
{
public:
    void Update(CWanted* wanted, float dt);
    void DrawMask(IDirect3DDevice9* device, render::PanelSurface const& surface, PanelRects const& panels,
        float panel_alpha, std::vector<render::BarVertex>& vertices) const;
    void DrawWave(IDirect3DDevice9* device, render::PanelSurface const& surface, PanelRects const& panels,
        float panel_alpha) const;

private:
    float m_progress = 0.0f; // target fill fraction within the current tier
    float m_display = 0.0f;  // smoothed fill fraction actually drawn
    float m_cycle = 0.0f;    // time within the current sweep cycle
    float m_phase = 1.0f;    // sweep head 0..1
    float m_wave_alpha = 0.0f; // wave visibility (fades out at the end of a sweep)
    float m_pulse = 0.0f;    // remaining time of the "star gained" white flash
    bool m_filling = false;  // on a gain, first fill the bar to full before the flash
    int m_last_level = -1;
};

// Info panel icons (clock, wanted star) draw with the solid bar shader bound.
void DrawInfoIcons(IDirect3DDevice9* device, render::PanelSurface const& surface, PanelRects const& panels,
    float panel_alpha, std::vector<render::BarVertex>& vertices);

// Info panel text (money, clock, wanted count) draws with the text render states bound.
void DrawInfoText(IDirect3DDevice9* device, render::TextRenderer& text, render::PanelSurface const& surface,
    PanelRects const& panels, float panel_alpha);

}
