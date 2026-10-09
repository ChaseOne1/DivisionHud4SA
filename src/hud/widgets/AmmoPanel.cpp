#include "hud/widgets/AmmoPanel.hpp"

#include "plugin.h"

#include "CPed.h"
#include "CWeapon.h"
#include "CWeaponInfo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>

#include "core/Config.hpp"
#include "hud/widgets/Layout.hpp"
#include "render/Primitives.hpp"

namespace division::hud {
using namespace render;

namespace {
    bool IsOccupiedSlot(CPed* ped, int slot)
    {
        // Skill 1 is the base weapon row (skill 0 indexes a different weapon entry).
        CWeaponInfo* info = CWeaponInfo::GetWeaponInfo(ped->m_aWeapons[slot].m_eWeaponType, 1);
        return info && static_cast<int>(info->m_nSlot) == slot;
    }

    int FindWeaponSlot(CPed* ped, int from, int step)
    {
        int const count = static_cast<int>(std::size(ped->m_aWeapons));
        for (int i = 1; i < count; ++i) {
            int const slot = ((from + step * i) % count + count) % count;
            if (IsOccupiedSlot(ped, slot)) return slot;
        }
        return -1;
    }

    // Per-channel colour blend, used to cross-fade the clip / reserve numbers on reload.
    D3DCOLOR LerpColor(D3DCOLOR a, D3DCOLOR b, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        auto channel = [t](unsigned int from, unsigned int to) {
            float const value = static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * t;
            return static_cast<unsigned int>(value + 0.5f) & 0xffu;
        };
        unsigned int const r = channel((a >> 16) & 0xffu, (b >> 16) & 0xffu);
        unsigned int const g = channel((a >> 8) & 0xffu, (b >> 8) & 0xffu);
        unsigned int const bl = channel(a & 0xffu, b & 0xffu);
        unsigned int const al = channel((a >> 24) & 0xffu, (b >> 24) & 0xffu);
        return (al << 24) | (r << 16) | (g << 8) | bl;
    }

    // Draws one ammo number, centred inside its zone, with the optional drop shadow.
    // `min_digits` zero-pads the value (e.g. 2 -> "07"); `pop` lifts the number off
    // the card toward the camera (see PanelFrame).
    void DrawAmmoNumber(IDirect3DDevice9* device, TextRenderer& text, PanelFrame const& frame,
        Rect const& zone, int value, int min_digits, float size_fraction, float offset_x, float offset_y,
        D3DCOLOR color, float alpha, float pop)
    {
        int tex_w = 0;
        int tex_h = 0;
        char buffer[16] { };
        std::snprintf(buffer, sizeof(buffer), "%0*d", std::max(1, min_digits), value);
        IDirect3DTexture9* texture = text.Get(device, buffer, tex_w, tex_h);
        if (!texture || tex_w <= 0 || tex_h <= 0) return;

        float const zone_w = zone.x1 - zone.x0;
        float const zone_h = zone.y1 - zone.y0;
        if (zone_w <= 0.0f || zone_h <= 0.0f) return;

        float const texture_aspect = (static_cast<float>(tex_w) / static_cast<float>(tex_h)) * UnitAspect();
        float glyph_height = zone_h * size_fraction;
        float glyph_width = glyph_height * texture_aspect;
        if (glyph_width > zone_w) {
            glyph_width = zone_w;
            glyph_height = glyph_width / texture_aspect;
        }

        float const x_l = zone.x0 + (zone_w - glyph_width) * 0.5f + offset_x;
        float const x_r = x_l + glyph_width;
        float const y_center = (zone.y0 + zone.y1) * 0.5f + offset_y;
        float const y_top = y_center + glyph_height * 0.5f;
        float const y_bottom = y_center - glyph_height * 0.5f;

        ScreenPoint const tl = frame.Map(x_l, y_top, pop);
        ScreenPoint const tr = frame.Map(x_r, y_top, pop);
        ScreenPoint const bl = frame.Map(x_l, y_bottom, pop);
        ScreenPoint const br = frame.Map(x_r, y_bottom, pop);

        device->SetTexture(0, texture);
        ScreenPoint const quad[4] = { tl, tr, bl, br };
        float const uv[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };

        if (core::cfg.PanelAmmoShadowOffset > 0.0f) {
            // Shadow stays flat on the card (un-popped) so it does not move with the number.
            float const offset = core::cfg.PanelAmmoShadowOffset;
            ScreenPoint const stl = frame.Map(x_l, y_top, 0.0f);
            ScreenPoint const str = frame.Map(x_r, y_top, 0.0f);
            ScreenPoint const sbl = frame.Map(x_l, y_bottom, 0.0f);
            ScreenPoint const sbr = frame.Map(x_r, y_bottom, 0.0f);
            ScreenPoint const shadow_quad[4] = {
                { stl.x + offset, stl.y + offset },
                { str.x + offset, str.y + offset },
                { sbl.x + offset, sbl.y + offset },
                { sbr.x + offset, sbr.y + offset },
            };
            DrawTexturedQuad(device, shadow_quad, uv, ScaleColor(core::cfg.PanelAmmoShadowColor, alpha));
        }

        DrawTexturedQuad(device, quad, uv, ScaleColor(color, alpha));
    }
}

void AmmoPanel::Update(CPed* ped, float dt)
{
    bool firing = false;
    bool reloading = false;
    if (ped) {
        firing = ped->bFiringWeapon;
        reloading = (ped->m_aWeapons[ped->m_nSelectedWepSlot].m_nState == WEAPONSTATE_RELOADING);
    }

    // bFiringWeapon pulses once per shot, so its rising edge is a shot event.
    if (firing && !m_firing_prev)
        m_fire_kick = 1.0f;
    m_firing_prev = firing;

    float const decay = std::max(0.0f, core::cfg.PanelAmmoFirePopDecay);
    m_fire_kick *= std::exp(-decay * dt);
    if (m_fire_kick < 0.001f)
        m_fire_kick = 0.0f;

    float const target = reloading ? 1.0f : 0.0f;
    float const step = dt / std::max(0.01f, core::cfg.PanelAmmoReloadTransition);
    m_reload_blend = (m_reload_blend < target) ? std::min(target, m_reload_blend + step)
                                               : std::max(target, m_reload_blend - step);
}

void AmmoPanel::Draw(IDirect3DDevice9* device, TextRenderer& text, PanelFrame const& frame,
    AmmoZones const& zones, CPed* ped, float panel_alpha) const
{
    if (!ped) return;

    int const current_slot = ped->m_nSelectedWepSlot;
    CWeapon const& current_weapon = ped->m_aWeapons[current_slot];
    int const prev_slot = FindWeaponSlot(ped, current_slot, -1);
    int const next_slot = FindWeaponSlot(ped, current_slot, 1);

    // During a reload the clip number falls back onto the card while the reserve
    // number lifts off it, and the two colours swap over the transition.
    float const blend = m_reload_blend;
    float const clip_pop = (core::cfg.PanelAmmoClipPop + core::cfg.PanelAmmoFirePop * m_fire_kick) * (1.0f - blend);
    float const reserve_pop = core::cfg.PanelAmmoClipPop * blend;
    D3DCOLOR const clip_color = LerpColor(core::cfg.PanelAmmoClipColor, core::cfg.PanelAmmoReserveColor, blend);
    D3DCOLOR const reserve_color = LerpColor(core::cfg.PanelAmmoReserveColor, core::cfg.PanelAmmoClipColor, blend);

    bool const reloading = (current_weapon.m_nState == WEAPONSTATE_RELOADING);
    D3DCOLOR const side_color = reloading ? core::cfg.PanelAmmoSideReloadColor : core::cfg.PanelAmmoSideColor;

    DrawAmmoNumber(device, text, frame, zones.clip, static_cast<int>(current_weapon.m_nAmmoInClip), 2,
        core::cfg.PanelAmmoClipSize, core::cfg.PanelAmmoClipOffsetX, core::cfg.PanelAmmoClipOffsetY, clip_color, panel_alpha,
        clip_pop);
    DrawAmmoNumber(device, text, frame, zones.reserve,
        static_cast<int>(current_weapon.m_nAmmoTotal - current_weapon.m_nAmmoInClip), 2, core::cfg.PanelAmmoReserveSize,
        core::cfg.PanelAmmoReserveOffsetX, core::cfg.PanelAmmoReserveOffsetY, reserve_color, panel_alpha, reserve_pop);
    if (prev_slot >= 0)
        DrawAmmoNumber(device, text, frame, zones.prev, static_cast<int>(ped->m_aWeapons[prev_slot].m_nAmmoInClip), 2,
            core::cfg.PanelAmmoSideSize, core::cfg.PanelAmmoPrevOffsetX, core::cfg.PanelAmmoPrevOffsetY, side_color, panel_alpha,
            0.0f);
    if (next_slot >= 0)
        DrawAmmoNumber(device, text, frame, zones.next, static_cast<int>(ped->m_aWeapons[next_slot].m_nAmmoInClip), 2,
            core::cfg.PanelAmmoSideSize, core::cfg.PanelAmmoNextOffsetX, core::cfg.PanelAmmoNextOffsetY, side_color, panel_alpha,
            0.0f);
}

}