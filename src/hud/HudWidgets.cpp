#include "hud/HudWidgets.hpp"

#include "plugin.h"

#include "CPed.h"
#include "CPedIntelligence.h"
#include "CWeaponInfo.h"
#include "CPlayerData.h"
#include "CStats.h"
#include "CWorld.h"
#include "CWanted.h"
#include "CClock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iterator>
#include <string>

#include "core/Config.hpp"
#include "render/TextRenderer.hpp"

namespace division::hud {
using namespace render;

namespace {
    constexpr float STAMINA_FLOOR = -150.0f;
    constexpr float STAMINA_NOT_FULL_EPSILON = 0.005f;
    constexpr float MIN_FADE_DURATION = 0.02f;
    constexpr float MAX_FADE_STEP = 0.5f;
    // Below this alpha a bar is considered invisible and skipped.
    constexpr float INVISIBLE_ALPHA = 0.001f;
    // Minimum health delta treated as a real change (filters float noise).
    constexpr float HEALTH_CHANGE_EPSILON = 0.01f;

    float AdvanceFade(float alpha, float target, float dt, float fade_in, float fade_out)
    {
        float const fade = (alpha < target) ? std::max(MIN_FADE_DURATION, fade_in) : std::max(MIN_FADE_DURATION, fade_out);
        float const step = std::min(MAX_FADE_STEP, dt / fade);
        if (alpha < target) return std::min(1.0f, alpha + step);
        if (alpha > target) return std::max(0.0f, alpha - step);
        return alpha;
    }

    float UnitAspect()
    {
        return core::PanelHeight / std::max(core::PanelLength, 0.0001f);
    }

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

    bool DrawString(IDirect3DDevice9* device, TextRenderer& text, PanelSurface const& surface, std::string const& str,
        float x_left, float y_center, float glyph_height, D3DCOLOR color)
    {
        int tex_w = 0;
        int tex_h = 0;
        IDirect3DTexture9* texture = text.Get(device, str, tex_w, tex_h);
        if (!texture || tex_w <= 0 || tex_h <= 0) return false;

        float const glyph_width = glyph_height * (static_cast<float>(tex_w) / static_cast<float>(tex_h)) * UnitAspect();
        ScreenPoint const tl = surface.Map(x_left, y_center + glyph_height * 0.5f);
        ScreenPoint const tr = surface.Map(x_left + glyph_width, y_center + glyph_height * 0.5f);
        ScreenPoint const bl = surface.Map(x_left, y_center - glyph_height * 0.5f);
        ScreenPoint const br = surface.Map(x_left + glyph_width, y_center - glyph_height * 0.5f);

        device->SetTexture(0, texture);
        ScreenPoint const quad[4] = { tl, tr, bl, br };
        float const uv[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };
        DrawTexturedQuad(device, quad, uv, color);
        return true;
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

        if (core::PanelAmmoShadowOffset > 0.0f) {
            // Shadow stays flat on the card (un-popped) so it does not move with the number.
            float const offset = core::PanelAmmoShadowOffset;
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
            DrawTexturedQuad(device, shadow_quad, uv, ScaleColor(core::PanelAmmoShadowColor, alpha));
        }

        DrawTexturedQuad(device, quad, uv, ScaleColor(color, alpha));
    }
}

void Vitals::Update(CPed* ped, float dt)
{
    m_max_stamina = CStats::GetFatAndMuscleModifier(STAT_MOD_TIME_CAN_RUN);
    m_max_air = CStats::GetFatAndMuscleModifier(STAT_MOD_AIR_IN_LUNG);

    if (ped) {
        float const health = ped->m_fHealth;
        if (!m_health_initialised) {
            m_health_initialised = true;
            m_health_prev = health;
            m_health_chip = health;
        }

        float const delta = health - m_health_prev;
        bool const damaged = delta < -HEALTH_CHANGE_EPSILON;
        bool const healed = delta > HEALTH_CHANGE_EPSILON;

        // Damage indicator: the red trailing bar stays where health was before the
        // hit(s) and catches up after a short hold. Consecutive hits keep the highest
        // recent value and restart the hold, so a burst reads as one indicator.
        if (damaged) {
            m_health_chip = std::max(m_health_chip, m_health_prev);
            m_health_chip_hold = core::PanelVitalsHealthChipHold;
        }
        if (m_health_chip < health)
            m_health_chip = health;

        float const max_health = static_cast<float>(CWorld::Players[CWorld::PlayerInFocus].m_nMaxHealth);
        m_health_chip_hold = std::max(0.0f, m_health_chip_hold - dt);
        if (m_health_chip_hold <= 0.0f)
            m_health_chip = std::max(health,
                m_health_chip - core::PanelVitalsHealthChipSpeed * std::max(1.0f, max_health) * dt);

        // Bar grows taller whenever health changes, then settles back.
        if (damaged || healed)
            m_health_pulse_hold = core::PanelVitalsHealthPulseHold;
        m_health_pulse_hold = std::max(0.0f, m_health_pulse_hold - dt);
        m_health_pulse = AdvanceFade(m_health_pulse, (m_health_pulse_hold > 0.0f) ? 1.0f : 0.0f, dt,
            core::PanelVitalsHealthPulseFadeIn, core::PanelVitalsHealthPulseFadeOut);

        m_health_prev = health;
    }

    if (ped && ped->m_pPlayerData) {
        float const stamina = ped->m_pPlayerData->m_fTimeCanRun;
        float const epsilon = std::max(0.5f, m_max_stamina * STAMINA_NOT_FULL_EPSILON);
        bool const not_full = (m_max_stamina > 0.0f) && (stamina < m_max_stamina - epsilon);

        m_stamina_hold = not_full ? core::PanelVitalsStaminaHold : std::max(0.0f, m_stamina_hold - dt);
        m_stamina_alpha = AdvanceFade(m_stamina_alpha, (m_stamina_hold > 0.0f) ? 1.0f : 0.0f, dt,
            core::PanelVitalsStaminaFadeIn, core::PanelVitalsStaminaFadeOut);
    }

    if (ped && ped->m_pPlayerData && ped->m_pIntelligence) {
        bool active = false;
        if (m_max_air > 0.0f) {
            bool const swimming = ped->m_pIntelligence->GetTaskSwim() != nullptr;
            active = swimming || (ped->m_pPlayerData->m_fBreath < m_max_air);
        }

        m_breath_hold = active ? core::PanelVitalsBreathHold : std::max(0.0f, m_breath_hold - dt);
        m_breath_alpha = AdvanceFade(m_breath_alpha, (m_breath_hold > 0.0f) ? 1.0f : 0.0f, dt,
            core::PanelVitalsStaminaFadeIn, core::PanelVitalsStaminaFadeOut);
    }

    // Animate each bar's presence so appearing/disappearing bars reflow the stack smoothly
    // instead of snapping.
    {
        float const max_armour = static_cast<float>(CWorld::Players[CWorld::PlayerInFocus].m_nMaxArmour);
        float const unit = std::max(0.0001f, core::PanelVitalsArmorUnit);
        bool const armor_present = (max_armour > 0.0f) && ped && (std::lround(ped->m_fArmour / unit) > 0);

        float const targets[BAR_COUNT] = {
            1.0f,
            armor_present ? 1.0f : 0.0f,
            (m_max_stamina > 0.0f && m_stamina_alpha > INVISIBLE_ALPHA) ? 1.0f : 0.0f,
            (m_max_air > 0.0f && m_breath_alpha > INVISIBLE_ALPHA) ? 1.0f : 0.0f,
        };
        for (int i = 0; i < BAR_COUNT; ++i) {
            m_bar_presence[i] = AdvanceFade(m_bar_presence[i], targets[i], dt,
                core::PanelVitalsMoveFade, core::PanelVitalsMoveFade);
        }
    }
}

void Vitals::Draw(IDirect3DDevice9* device, PanelSurface const& surface, float panel_alpha,
    std::vector<BarVertex>& vertices) const
{
    CPed* ped = FindPlayerPed(0);
    if (!ped) return;

    CPlayerInfo& info = CWorld::Players[CWorld::PlayerInFocus];
    float const max_health = static_cast<float>(info.m_nMaxHealth);
    float const max_armour = static_cast<float>(info.m_nMaxArmour);
    float const max_stamina = m_max_stamina;
    float const max_air = m_max_air;

    float const length = std::clamp(core::PanelVitalsLength, 0.05f, 1.0f);
    float const x0 = (1.0f - length) * 0.5f;

    float const unit = std::max(0.0001f, core::PanelVitalsArmorUnit);
    int armor_total = (max_armour > 0.0f) ? static_cast<int>(std::lround(max_armour / unit)) : 1;
    if (armor_total < 1)
        armor_total = 1;
    int armor_filled = (max_armour > 0.0f) ? static_cast<int>(std::lround(ped->m_fArmour / unit)) : 0;
    armor_filled = std::clamp(armor_filled, 0, armor_total);

    float const gap = core::PanelVitalsGap;
    float const health_thickness = core::PanelVitalsThicknessHealth
        * (1.0f + core::PanelVitalsHealthPulseScale * std::max(0.0f, m_health_pulse));

    D3DCOLOR const health_color = ScaleColor(core::PanelVitalsHealthColor, panel_alpha);
    D3DCOLOR const track_color = ScaleColor(core::PanelVitalsHealthTrackColor, panel_alpha);
    D3DCOLOR const chip_color = ScaleColor(core::PanelVitalsHealthChipColor, panel_alpha);
    D3DCOLOR const armor_color = ScaleColor(core::PanelVitalsArmorColor, panel_alpha);
    D3DCOLOR const stamina_color = ScaleColor(core::PanelVitalsStaminaColor, panel_alpha * m_stamina_alpha);
    D3DCOLOR const breath_color = ScaleColor(core::PanelVitalsBreathColor, panel_alpha * m_breath_alpha);
    D3DCOLOR const cursor_color = ScaleColor(core::PanelVitalsCursorColor, panel_alpha);
    D3DCOLOR const stamina_cursor_color = ScaleColor(core::PanelVitalsCursorColor, panel_alpha * m_stamina_alpha);
    D3DCOLOR const breath_cursor_color = ScaleColor(core::PanelVitalsCursorColor, panel_alpha * m_breath_alpha);

    vertices.clear();
    vertices.reserve(640);

    auto add_bar = [&](float y_bottom, float y_top, float frac, D3DCOLOR color) {
        if (frac <= 0.0f) return;
        float const right = x0 + length * std::clamp(frac, 0.0f, 1.0f);
        AppendAARect(vertices, surface, x0, y_bottom, right, y_top, color);
    };

    auto add_cursor = [&](float y_bottom, float y_top, float frac, D3DCOLOR color) {
        float const center_x = x0 + length * std::clamp(frac, 0.0f, 1.0f);
        float const half_w = core::PanelVitalsCursorWidth * 0.5f;
        float const center_y = (y_bottom + y_top) * 0.5f;
        float const half_h = (y_top - y_bottom) * 0.5f * core::PanelVitalsCursorScale;
        AppendAARect(vertices, surface, center_x - half_w, center_y - half_h, center_x + half_w, center_y + half_h, color);
    };

    // Small square under the cursor stem (same width as the stem), so the health marker reads like
    // an upside-down "i". Height derives from the card aspect so it stays a square on screen.
    auto add_cursor_dot = [&](float y_bottom, float y_top, float frac, D3DCOLOR color) {
        float const half_w = core::PanelVitalsCursorWidth * 0.5f;
        if (half_w <= 0.0f) return;
        float const half_h = half_w / UnitAspect();
        float const center_x = x0 + length * std::clamp(frac, 0.0f, 1.0f);
        float const center_y = (y_bottom + y_top) * 0.5f;
        float const stem_bottom = center_y - (y_top - y_bottom) * 0.5f * core::PanelVitalsCursorScale;
        float const dot_top = stem_bottom - core::PanelVitalsCursorDotGap;
        AppendAARect(vertices, surface, center_x - half_w, dot_top - 2.0f * half_h, center_x + half_w, dot_top, color);
    };

    float const health_frac = (max_health > 0.0f) ? ped->m_fHealth / max_health : 0.0f;

    // Bars are stacked bottom-up from the card edge. Each bar's animated presence scales its slot
    // (height + gap) and fades it, so appearing/disappearing bars slide instead of snapping; a
    // fully hidden bar leaves no gap, so e.g. breath sits right above health when armour and
    // stamina are absent. To add a bar, append an entry here; ordering is the stack order.
    struct StackBar
    {
        float thickness;
        float presence;
        std::function<void(float, float, float)> render;
    };

    StackBar const bars[] = {
        { health_thickness, m_bar_presence[BAR_HEALTH], [&](float bottom, float top, float) {
             // Red trailing segment spans the recently lost health behind the live bar.
             float const chip_frac = std::clamp(m_health_chip / max_health, health_frac, 1.0f);
             add_bar(bottom, top, 1.0f, track_color);
             add_bar(bottom, top, chip_frac, chip_color);
             add_bar(bottom, top, health_frac, health_color);
             add_cursor(bottom, top, health_frac, cursor_color);
             add_cursor_dot(bottom, top, health_frac, cursor_color);
         } },
        { core::PanelVitalsThicknessArmor, m_bar_presence[BAR_ARMOR], [&](float bottom, float top, float w) {
             float const segment_width = length / static_cast<float>(armor_total);
             float const segment_gap = core::PanelVitalsArmorGap;
             for (int i = 0; i < armor_filled; ++i) {
                 float const seg_x0 = x0 + segment_width * i + segment_gap * 0.5f;
                 float const seg_x1 = x0 + segment_width * (i + 1) - segment_gap * 0.5f;
                 if (seg_x1 <= seg_x0) continue;
                 AppendAARect(vertices, surface, seg_x0, bottom, seg_x1, top, ScaleColor(armor_color, w));
             }
         } },
        { core::PanelVitalsThicknessStamina, m_bar_presence[BAR_STAMINA], [&](float bottom, float top, float w) {
             float const stamina_frac = std::clamp(
                 (ped->m_pPlayerData->m_fTimeCanRun - STAMINA_FLOOR) / (max_stamina - STAMINA_FLOOR), 0.0f, 1.0f);
             add_bar(bottom, top, stamina_frac, ScaleColor(stamina_color, w));
             add_cursor(bottom, top, stamina_frac, ScaleColor(stamina_cursor_color, w));
         } },
        { core::PanelVitalsThicknessStamina, m_bar_presence[BAR_BREATH], [&](float bottom, float top, float w) {
             float const breath_frac = std::clamp(ped->m_pPlayerData->m_fBreath / max_air, 0.0f, 1.0f);
             add_bar(bottom, top, breath_frac, ScaleColor(breath_color, w));
             add_cursor(bottom, top, breath_frac, ScaleColor(breath_cursor_color, w));
         } },
    };

    float bottom = 1.0f + core::PanelVitalsOffsetY;
    for (StackBar const& bar : bars) {
        if (bar.presence <= INVISIBLE_ALPHA) continue;
        float const height = bar.thickness * bar.presence;
        bar.render(bottom, bottom + height, bar.presence);
        bottom += (bar.thickness + gap) * bar.presence;
    }

    if (!vertices.empty())
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(vertices.size() / 3), vertices.data(),
            sizeof(BarVertex));
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

    float const decay = std::max(0.0f, core::PanelAmmoFirePopDecay);
    m_fire_kick *= std::exp(-decay * dt);
    if (m_fire_kick < 0.001f)
        m_fire_kick = 0.0f;

    float const target = reloading ? 1.0f : 0.0f;
    float const step = dt / std::max(0.01f, core::PanelAmmoReloadTransition);
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
    float const clip_pop = (core::PanelAmmoClipPop + core::PanelAmmoFirePop * m_fire_kick) * (1.0f - blend);
    float const reserve_pop = core::PanelAmmoClipPop * blend;
    D3DCOLOR const clip_color = LerpColor(core::PanelAmmoClipColor, core::PanelAmmoReserveColor, blend);
    D3DCOLOR const reserve_color = LerpColor(core::PanelAmmoReserveColor, core::PanelAmmoClipColor, blend);

    bool const reloading = (current_weapon.m_nState == WEAPONSTATE_RELOADING);
    D3DCOLOR const side_color = reloading ? core::PanelAmmoSideReloadColor : core::PanelAmmoSideColor;

    DrawAmmoNumber(device, text, frame, zones.clip, static_cast<int>(current_weapon.m_nAmmoInClip), 2,
        core::PanelAmmoClipSize, core::PanelAmmoClipOffsetX, core::PanelAmmoClipOffsetY, clip_color, panel_alpha,
        clip_pop);
    DrawAmmoNumber(device, text, frame, zones.reserve,
        static_cast<int>(current_weapon.m_nAmmoTotal - current_weapon.m_nAmmoInClip), 2, core::PanelAmmoReserveSize,
        core::PanelAmmoReserveOffsetX, core::PanelAmmoReserveOffsetY, reserve_color, panel_alpha, reserve_pop);
    if (prev_slot >= 0)
        DrawAmmoNumber(device, text, frame, zones.prev, static_cast<int>(ped->m_aWeapons[prev_slot].m_nAmmoInClip), 2,
            core::PanelAmmoSideSize, core::PanelAmmoPrevOffsetX, core::PanelAmmoPrevOffsetY, side_color, panel_alpha,
            0.0f);
    if (next_slot >= 0)
        DrawAmmoNumber(device, text, frame, zones.next, static_cast<int>(ped->m_aWeapons[next_slot].m_nAmmoInClip), 2,
            core::PanelAmmoSideSize, core::PanelAmmoNextOffsetX, core::PanelAmmoNextOffsetY, side_color, panel_alpha,
            0.0f);
}

void DrawInfoIcons(IDirect3DDevice9* device, PanelSurface const& surface, PanelRects const& panels,
    float panel_alpha, std::vector<BarVertex>& vertices)
{
    float const unit_aspect = UnitAspect();
    D3DCOLOR const color = ScaleColor(core::PanelHudIconColor, panel_alpha);

    auto icon_radius = [&](Rect const& r) {
        return (r.y1 - r.y0) * core::PanelHudIconSize * 0.5f;
    };

    vertices.clear();
    vertices.reserve(768);

    {
        Rect const& r = panels.resource;
        float const cx = r.x0 + (r.x1 - r.x0) * core::PanelClockIconX;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const ry = icon_radius(r);
        float const rx = ry * unit_aspect;
        AppendRing(vertices, surface, cx, cy, rx, ry, 0.87f, color);
        AppendSolidRect(vertices, surface, cx - rx * 0.08f, cy, cx + rx * 0.08f, cy + ry * 0.52f, color);
        AppendSolidRect(vertices, surface, cx, cy - ry * 0.055f, cx + rx * 0.70f, cy + ry * 0.055f, color);
    }

    {
        Rect const& r = panels.resource;
        float const cx = r.x0 + (r.x1 - r.x0) * core::PanelWantedIconX;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const ry = icon_radius(r);
        AppendStar(vertices, surface, cx, cy, ry * unit_aspect, ry, color);
    }

    if (!vertices.empty())
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(vertices.size() / 3), vertices.data(),
            sizeof(BarVertex));
}

void DrawInfoText(IDirect3DDevice9* device, TextRenderer& text, PanelSurface const& surface,
    PanelRects const& panels, float panel_alpha)
{
    D3DCOLOR const color = ScaleColor(core::PanelHudTextColor, panel_alpha);
    CPlayerInfo& info = CWorld::Players[CWorld::PlayerInFocus];

    {
        Rect const& r = panels.skill;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const text_height = (r.y1 - r.y0) * core::PanelHudTextSize;
        float const icon_height = (r.y1 - r.y0) * core::PanelHudIconSize;
        char buffer[32] { };
        std::snprintf(buffer, sizeof(buffer), "%0*d", std::max(1, core::PanelMoneyDigits), info.m_nMoney);
        DrawString(device, text, surface, "$", r.x0 + (r.x1 - r.x0) * core::PanelSkillIconX, cy, icon_height,
            color);
        DrawString(device, text, surface, buffer, r.x0 + (r.x1 - r.x0) * core::PanelSkillValueX, cy, text_height,
            color);
    }

    {
        Rect const& r = panels.resource;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const text_height = (r.y1 - r.y0) * core::PanelHudTextSize;
        char buffer[16] { };
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", CClock::ms_nGameClockHours, CClock::ms_nGameClockMinutes);
        DrawString(device, text, surface, buffer, r.x0 + (r.x1 - r.x0) * core::PanelClockValueX, cy, text_height,
            color);
    }

    {
        Rect const& r = panels.resource;
        float const cy = (r.y0 + r.y1) * 0.5f;
        float const text_height = (r.y1 - r.y0) * core::PanelHudTextSize;
        unsigned int wanted = 0;
        if (info.m_PlayerData.m_pWanted)
            wanted = info.m_PlayerData.m_pWanted->m_nWantedLevel;
        char buffer[16] { };
        std::snprintf(buffer, sizeof(buffer), "%u", wanted);
        DrawString(device, text, surface, buffer, r.x0 + (r.x1 - r.x0) * core::PanelWantedValueX, cy, text_height,
            color);
    }
}
}
