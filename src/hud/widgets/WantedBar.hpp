#pragma once

#include <d3d9.h>
#include <vector>

#include "render/Surface.hpp"
#include "hud/widgets/Layout.hpp"

class CWanted;

namespace division::hud {

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

}