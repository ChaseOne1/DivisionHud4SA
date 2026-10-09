#include "hud/widgets/Vitals.hpp"

#include "plugin.h"

#include "common.h"
#include "CPed.h"
#include "CPedIntelligence.h"
#include "CPlayerData.h"
#include "CStats.h"
#include "CWorld.h"

#include <algorithm>
#include <cmath>

#include "core/Config.hpp"
#include "core/Animation.hpp"
#include "hud/widgets/Layout.hpp"
#include "render/Primitives.hpp"

namespace division::hud {
using namespace render;

namespace {
    constexpr float STAMINA_FLOOR = -150.0f;
    constexpr float STAMINA_NOT_FULL_EPSILON = 0.005f;
    // Below this alpha a bar is considered invisible and skipped.
    constexpr float INVISIBLE_ALPHA = 0.001f;
    // Minimum health delta treated as a real change (filters float noise).
    constexpr float HEALTH_CHANGE_EPSILON = 0.01f;
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
            m_health_chip_hold = core::cfg.PanelVitalsHealthChipHold;
        }
        if (m_health_chip < health)
            m_health_chip = health;

        float const max_health = static_cast<float>(CWorld::Players[CWorld::PlayerInFocus].m_nMaxHealth);
        m_health_chip_hold = std::max(0.0f, m_health_chip_hold - dt);
        if (m_health_chip_hold <= 0.0f)
            m_health_chip = std::max(health,
                m_health_chip - core::cfg.PanelVitalsHealthChipSpeed * std::max(1.0f, max_health) * dt);

        // Bar grows taller whenever health changes, then settles back.
        if (damaged || healed)
            m_health_pulse_hold = core::cfg.PanelVitalsHealthPulseHold;
        m_health_pulse_hold = std::max(0.0f, m_health_pulse_hold - dt);
        m_health_pulse = core::AdvanceFade(m_health_pulse, (m_health_pulse_hold > 0.0f) ? 1.0f : 0.0f, dt,
            core::cfg.PanelVitalsHealthPulseFadeIn, core::cfg.PanelVitalsHealthPulseFadeOut);

        m_health_prev = health;
    }

    if (ped && ped->m_pPlayerData) {
        float const stamina = ped->m_pPlayerData->m_fTimeCanRun;
        float const epsilon = std::max(0.5f, m_max_stamina * STAMINA_NOT_FULL_EPSILON);
        bool const not_full = (m_max_stamina > 0.0f) && (stamina < m_max_stamina - epsilon);

        m_stamina_hold = not_full ? core::cfg.PanelVitalsStaminaHold : std::max(0.0f, m_stamina_hold - dt);
        m_stamina_alpha = core::AdvanceFade(m_stamina_alpha, (m_stamina_hold > 0.0f) ? 1.0f : 0.0f, dt,
            core::cfg.PanelVitalsStaminaFadeIn, core::cfg.PanelVitalsStaminaFadeOut);
    }

    if (ped && ped->m_pPlayerData && ped->m_pIntelligence) {
        bool active = false;
        if (m_max_air > 0.0f) {
            bool const swimming = ped->m_pIntelligence->GetTaskSwim() != nullptr;
            active = swimming || (ped->m_pPlayerData->m_fBreath < m_max_air);
        }

        m_breath_hold = active ? core::cfg.PanelVitalsBreathHold : std::max(0.0f, m_breath_hold - dt);
        m_breath_alpha = core::AdvanceFade(m_breath_alpha, (m_breath_hold > 0.0f) ? 1.0f : 0.0f, dt,
            core::cfg.PanelVitalsStaminaFadeIn, core::cfg.PanelVitalsStaminaFadeOut);
    }

    // Animate each bar's presence so appearing/disappearing bars reflow the stack smoothly
    // instead of snapping.
    {
        float const max_armour = static_cast<float>(CWorld::Players[CWorld::PlayerInFocus].m_nMaxArmour);
        float const unit = std::max(0.0001f, core::cfg.PanelVitalsArmorUnit);
        bool const armor_present = (max_armour > 0.0f) && ped && (std::lround(ped->m_fArmour / unit) > 0);

        float const targets[BAR_COUNT] = {
            1.0f,
            armor_present ? 1.0f : 0.0f,
            (m_max_stamina > 0.0f && m_stamina_alpha > INVISIBLE_ALPHA) ? 1.0f : 0.0f,
            (m_max_air > 0.0f && m_breath_alpha > INVISIBLE_ALPHA) ? 1.0f : 0.0f,
        };
        for (int i = 0; i < BAR_COUNT; ++i) {
            m_bar_presence[i] = core::AdvanceFade(m_bar_presence[i], targets[i], dt,
                core::cfg.PanelVitalsMoveFade, core::cfg.PanelVitalsMoveFade);
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

    float const length = std::clamp(core::cfg.PanelVitalsLength, 0.05f, 1.0f);
    float const x0 = (1.0f - length) * 0.5f;

    float const unit = std::max(0.0001f, core::cfg.PanelVitalsArmorUnit);
    int armor_total = (max_armour > 0.0f) ? static_cast<int>(std::lround(max_armour / unit)) : 1;
    if (armor_total < 1)
        armor_total = 1;
    int armor_filled = (max_armour > 0.0f) ? static_cast<int>(std::lround(ped->m_fArmour / unit)) : 0;
    armor_filled = std::clamp(armor_filled, 0, armor_total);

    float const gap = core::cfg.PanelVitalsGap;
    float const health_thickness = core::cfg.PanelVitalsThicknessHealth
        * (1.0f + core::cfg.PanelVitalsHealthPulseScale * std::max(0.0f, m_health_pulse));

    D3DCOLOR const health_color = ScaleColor(core::cfg.PanelVitalsHealthColor, panel_alpha);
    D3DCOLOR const track_color = ScaleColor(core::cfg.PanelVitalsHealthTrackColor, panel_alpha);
    D3DCOLOR const chip_color = ScaleColor(core::cfg.PanelVitalsHealthChipColor, panel_alpha);
    D3DCOLOR const armor_color = ScaleColor(core::cfg.PanelVitalsArmorColor, panel_alpha);
    D3DCOLOR const stamina_color = ScaleColor(core::cfg.PanelVitalsStaminaColor, panel_alpha * m_stamina_alpha);
    D3DCOLOR const breath_color = ScaleColor(core::cfg.PanelVitalsBreathColor, panel_alpha * m_breath_alpha);
    D3DCOLOR const cursor_color = ScaleColor(core::cfg.PanelVitalsCursorColor, panel_alpha);
    D3DCOLOR const stamina_cursor_color = ScaleColor(core::cfg.PanelVitalsCursorColor, panel_alpha * m_stamina_alpha);
    D3DCOLOR const breath_cursor_color = ScaleColor(core::cfg.PanelVitalsCursorColor, panel_alpha * m_breath_alpha);

    vertices.clear();
    vertices.reserve(640);

    auto add_bar = [&](float y_bottom, float y_top, float frac, D3DCOLOR color) {
        if (frac <= 0.0f) return;
        float const right = x0 + length * std::clamp(frac, 0.0f, 1.0f);
        AppendAARect(vertices, surface, x0, y_bottom, right, y_top, color);
    };

    auto add_cursor = [&](float y_bottom, float y_top, float frac, D3DCOLOR color) {
        float const center_x = x0 + length * std::clamp(frac, 0.0f, 1.0f);
        float const half_w = core::cfg.PanelVitalsCursorWidth * 0.5f;
        float const center_y = (y_bottom + y_top) * 0.5f;
        float const half_h = (y_top - y_bottom) * 0.5f * core::cfg.PanelVitalsCursorScale;
        AppendAARect(vertices, surface, center_x - half_w, center_y - half_h, center_x + half_w, center_y + half_h, color);
    };

    // Small square under the cursor stem (same width as the stem), so the health marker reads like
    // an upside-down "i". Height derives from the card aspect so it stays a square on screen.
    auto add_cursor_dot = [&](float y_bottom, float y_top, float frac, D3DCOLOR color) {
        float const half_w = core::cfg.PanelVitalsCursorWidth * 0.5f;
        if (half_w <= 0.0f) return;
        float const half_h = half_w / UnitAspect();
        float const center_x = x0 + length * std::clamp(frac, 0.0f, 1.0f);
        float const center_y = (y_bottom + y_top) * 0.5f;
        float const stem_bottom = center_y - (y_top - y_bottom) * 0.5f * core::cfg.PanelVitalsCursorScale;
        float const dot_top = stem_bottom - core::cfg.PanelVitalsCursorDotGap;
        AppendAARect(vertices, surface, center_x - half_w, dot_top - 2.0f * half_h, center_x + half_w, dot_top, color);
    };

    float const health_frac = (max_health > 0.0f) ? ped->m_fHealth / max_health : 0.0f;

    // Bars are stacked bottom-up from the card edge. Each bar's animated presence scales its slot
    // (height + gap) and fades it, so appearing/disappearing bars slide instead of snapping; a
    // fully hidden bar leaves no gap, so e.g. breath sits right above health when armour and
    // stamina are absent. To add a bar, append an entry here; ordering is the stack order.
    enum class BarKind : int { Health, Armor, Stamina, Breath };

    struct StackBar
    {
        BarKind kind;
        float thickness;
        float presence;
    };

    StackBar const bars[] = {
        { BarKind::Health, health_thickness, m_bar_presence[BAR_HEALTH] },
        { BarKind::Armor, core::cfg.PanelVitalsThicknessArmor, m_bar_presence[BAR_ARMOR] },
        { BarKind::Stamina, core::cfg.PanelVitalsThicknessStamina, m_bar_presence[BAR_STAMINA] },
        { BarKind::Breath, core::cfg.PanelVitalsThicknessStamina, m_bar_presence[BAR_BREATH] },
    };

    float bottom = 1.0f + core::cfg.PanelVitalsOffsetY;
    for (StackBar const& bar : bars) {
        if (bar.presence <= INVISIBLE_ALPHA) continue;
        float const height = bar.thickness * bar.presence;
        float const w = bar.presence;
        switch (bar.kind) {
            case BarKind::Health: {
                // Red trailing segment spans the recently lost health behind the live bar.
                float const chip_frac = std::clamp(m_health_chip / max_health, health_frac, 1.0f);
                add_bar(bottom, bottom + height, 1.0f, track_color);
                add_bar(bottom, bottom + height, chip_frac, chip_color);
                add_bar(bottom, bottom + height, health_frac, health_color);
                add_cursor(bottom, bottom + height, health_frac, cursor_color);
                add_cursor_dot(bottom, bottom + height, health_frac, cursor_color);
                break;
            }
            case BarKind::Armor: {
                float const segment_width = length / static_cast<float>(armor_total);
                float const segment_gap = core::cfg.PanelVitalsArmorGap;
                for (int i = 0; i < armor_filled; ++i) {
                    float const seg_x0 = x0 + segment_width * i + segment_gap * 0.5f;
                    float const seg_x1 = x0 + segment_width * (i + 1) - segment_gap * 0.5f;
                    if (seg_x1 <= seg_x0) continue;
                    AppendAARect(vertices, surface, seg_x0, bottom, seg_x1, bottom + height, ScaleColor(armor_color, w));
                }
                break;
            }
            case BarKind::Stamina: {
                float const stamina_frac = std::clamp(
                    (ped->m_pPlayerData->m_fTimeCanRun - STAMINA_FLOOR) / (max_stamina - STAMINA_FLOOR), 0.0f, 1.0f);
                add_bar(bottom, bottom + height, stamina_frac, ScaleColor(stamina_color, w));
                add_cursor(bottom, bottom + height, stamina_frac, ScaleColor(stamina_cursor_color, w));
                break;
            }
            case BarKind::Breath: {
                float const breath_frac = std::clamp(ped->m_pPlayerData->m_fBreath / max_air, 0.0f, 1.0f);
                add_bar(bottom, bottom + height, breath_frac, ScaleColor(breath_color, w));
                add_cursor(bottom, bottom + height, breath_frac, ScaleColor(breath_cursor_color, w));
                break;
            }
        }
        bottom += (bar.thickness + gap) * bar.presence;
    }

    if (!vertices.empty())
        device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(vertices.size() / 3), vertices.data(),
            sizeof(BarVertex));
}

}