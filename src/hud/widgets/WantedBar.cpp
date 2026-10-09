#include "hud/widgets/WantedBar.hpp"

#include "plugin.h"

#include "CWanted.h"

#include <algorithm>
#include <cmath>

#include "core/Config.hpp"
#include "render/Primitives.hpp"

namespace division::hud {
using namespace render;

void WantedBar::Update(CWanted* wanted, float dt)
{
    if (!wanted) {
        m_progress = 0.0f;
        m_display = 0.0f;
        m_cycle = 0.0f;
        m_phase = 1.0f;
        m_wave_alpha = 0.0f;
        m_pulse = 0.0f;
        m_filling = false;
        m_last_level = -1;
        return;
    }

    int const level = std::min<int>(static_cast<int>(wanted->m_nWantedLevel), 6);
    bool const first = (m_last_level < 0);
    bool const gained = (!first && level > m_last_level);
    if (gained) {
        // Star gained: fill the bar to full first, then flash, then jump to zero.
        m_filling = true;
        m_pulse = 0.0f;
    }

    if (level == 0) {
        m_progress = 0.0f;
        m_display = 0.0f;
        m_cycle = 0.0f;
        m_phase = 1.0f;
        m_wave_alpha = 0.0f;
        m_pulse = 0.0f;
        m_filling = false;
        m_last_level = level;
        return;
    }

    // Chaos thresholds from CWanted::UpdateWantedLevel (fixed in the game).
    static constexpr unsigned int thresholds[7] = { 0, 50, 180, 550, 1200, 2400, 4600 };
    unsigned int const lower = thresholds[level];
    unsigned int const upper = (level < 6) ? thresholds[level + 1]
                                           : std::max<unsigned int>(CWanted::nMaximumWantedLevel, lower + 1);
    float progress = 0.0f;
    if (upper > lower) {
        float const value = static_cast<float>(wanted->m_nChaosLevel);
        progress = (value - static_cast<float>(lower)) / (static_cast<float>(upper) - static_cast<float>(lower));
    }
    m_progress = std::clamp(progress, 0.0f, 1.0f);
    m_last_level = level;

    float const rate = std::min(1.0f, dt * std::max(1.0f, core::cfg.PanelWantedBarSpeed));

    // The sweep cycle keeps advancing so it stays continuous across a gain; it is just
    // hidden while the bar fills / flashes.
    float const sweep = std::max(0.05f, core::cfg.PanelWantedWaveSweep);
    float const fade = std::max(0.0f, core::cfg.PanelWantedWaveFade);
    float const period = sweep + fade + std::max(0.0f, core::cfg.PanelWantedWaveInterval);
    m_cycle = std::fmod(m_cycle + dt, period);
    if (m_cycle < sweep) {
        m_phase = m_cycle / sweep;
        m_wave_alpha = 1.0f;
    } else if (m_cycle < sweep + fade) {
        m_phase = 1.0f;
        m_wave_alpha = (fade > 0.0f) ? 1.0f - (m_cycle - sweep) / fade : 0.0f;
    } else {
        m_phase = 1.0f;
        m_wave_alpha = 0.0f;
    }

    if (m_filling) {
        m_display += (1.0f - m_display) * rate;
        if (m_display >= 0.999f) {
            m_display = 1.0f;
            m_filling = false;
            m_pulse = std::max(0.05f, core::cfg.PanelWantedFullDuration);
        }
        m_wave_alpha = 0.0f;
        return;
    }

    if (m_pulse > 0.0f) {
        m_display = 1.0f; // hold at full while the flash plays
        m_pulse = std::max(0.0f, m_pulse - dt);
        if (m_pulse <= 0.0f)
            m_display = m_progress; // jump to the new tier's fill after the flash
        m_wave_alpha = 0.0f;
        return;
    }

    if (first)
        m_display = m_progress;
    else
        m_display += (m_progress - m_display) * rate;
}

void WantedBar::DrawMask(IDirect3DDevice9* device, PanelSurface const& surface, PanelRects const& panels,
    float panel_alpha, std::vector<BarVertex>& vertices) const
{
    vertices.clear();

    Rect const& r = panels.resource;
    float const x0 = r.x0 + (r.x1 - r.x0) * core::cfg.PanelWantedBarX0;
    float const full = r.x0 + (r.x1 - r.x0) * core::cfg.PanelWantedBarX1;
    if (full <= x0) return;
    vertices.reserve(12);

    // Breathing tint over the whole wanted area, ramping to the mask colour as the
    // wave sweeps (0..1 during the sweep, 1..0 during its fade).
    float const breath = m_wave_alpha * m_phase;
    if (breath > 0.001f && core::cfg.PanelWantedBreath > 0.0f) {
        float const t = std::clamp(breath, 0.0f, 1.0f);
        float const eased = t * t * (3.0f - 2.0f * t);
        AppendAARect(vertices, surface, x0, r.y0, full, r.y1,
            ScaleColor(core::cfg.PanelWantedBarColor, panel_alpha * eased * core::cfg.PanelWantedBreath));
    }

    if (m_pulse > 0.0f) {
        // "Star gained" flash: the whole wanted area turns white, then fades back.
        float const duration = std::max(0.05f, core::cfg.PanelWantedFullDuration);
        float const factor = std::clamp(m_pulse / duration, 0.0f, 1.0f);
        AppendAARect(vertices, surface, x0, r.y0, full, r.y1,
            ScaleColor(core::cfg.PanelWantedFullColor, panel_alpha * factor));
    } else if (m_display > 0.0f) {
        AppendAARect(vertices, surface, x0, r.y0, x0 + (full - x0) * m_display, r.y1,
            ScaleColor(core::cfg.PanelWantedBarColor, panel_alpha));
    }

    if (!vertices.empty())
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(vertices.size() / 3), vertices.data(),
            sizeof(BarVertex));
}

void WantedBar::DrawWave(IDirect3DDevice9* device, PanelSurface const& surface, PanelRects const& panels,
    float panel_alpha) const
{
    if (m_display <= 0.0f || m_wave_alpha <= 0.001f) return;

    Rect const& r = panels.resource;
    float const x0 = r.x0 + (r.x1 - r.x0) * core::cfg.PanelWantedBarX0;
    float const full = r.x0 + (r.x1 - r.x0) * core::cfg.PanelWantedBarX1;
    float const x1 = x0 + (full - x0) * m_display;
    if (x1 <= x0) return;

    ScreenPoint const tl = surface.Map(x0, r.y1);
    ScreenPoint const tr = surface.Map(x1, r.y1);
    ScreenPoint const bl = surface.Map(x0, r.y0);
    ScreenPoint const br = surface.Map(x1, r.y0);
    ScreenPoint const tr_full = surface.Map(full, r.y1);
    auto distance = [](ScreenPoint const& a, ScreenPoint const& b) {
        float const dx = a.x - b.x, dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    };
    float const height_px = distance(tl, bl);
    float const width_px = distance(tl, tr_full);

    float const params[4] = { m_phase * m_display, std::max(0.0001f, core::cfg.PanelWantedWaveWidth), height_px,
        width_px };
    device->SetPixelShaderConstantF(0, params, 1);

    D3DCOLOR const color = ScaleColor(core::cfg.PanelWantedWaveColor, panel_alpha * m_wave_alpha);
    Vertex const quad[4] = {
        { tl.x, tl.y, 0.0f, 1.0f, color, 0.0f, 0.0f },
        { tr.x, tr.y, 0.0f, 1.0f, color, m_display, 0.0f },
        { bl.x, bl.y, 0.0f, 1.0f, color, 0.0f, 1.0f },
        { br.x, br.y, 0.0f, 1.0f, color, m_display, 1.0f },
    };
    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));
}

}